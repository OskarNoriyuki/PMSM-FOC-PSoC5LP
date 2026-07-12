"""
plot_step_response.py — Visualizador em tempo real dos ensaios dinâmicos

Le pela serial os blocos emitidos pelo firmware (Inverter_Utils / vTestPrint):

    #TEST_BEGIN mode=SPEED unit=rpm
    0;500;-3
    5;500;123
    ...
    #TEST_END

Cada linha de dados traz  t_ms ; referencia ; medido.
A leitura roda em thread separada; a curva atualiza ao vivo conforme
as amostras chegam.

Requisitos:  pip install pyserial dearpygui matplotlib numpy
Uso:         python plot_step_response.py
"""

import threading
import queue
import time
import os
from dataclasses import dataclass, field

import serial
from serial.tools import list_ports
import dearpygui.dearpygui as dpg
import numpy as np


BAUD_OPTIONS = ["9600", "115200", "230400", "460800", "921600"]
DEFAULT_BAUD = "460800"

REF_COLOR = "red"   # cor da referencia na figura exportada
OUTPUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                          "Generated_StepResponse")

SETTLE_TOL = 0.05        # banda de acomodacao: 5% -> chega a 95% da referencia
STEADY_HALF_MS = 500     # janela de regime = ultima metade (0,5 s) do plateau

GUI_YLABEL = {"mA": "Iq (mA)", "rpm": "Velocidade (rpm)", "deg": "Posição (graus)"}
FIG_YLABEL = {"mA": r"$I_q$ (mA)", "rpm": "Velocidade (rpm)", "deg": "Posição (graus)"}


@dataclass
class TestRun:
    mode: str = "?"
    unit: str = "?"
    t: list = field(default_factory=list)
    ref: list = field(default_factory=list)
    meas: list = field(default_factory=list)
    complete: bool = False


# --------------------------------------------------------------------------- #
#  Serial worker (thread)
# --------------------------------------------------------------------------- #
class SerialWorker(threading.Thread):
    def __init__(self, port, baud):
        super().__init__(daemon=True)
        self.port = port
        self.baud = int(baud)
        self._stop = threading.Event()
        self._lock = threading.Lock()
        self.status = queue.Queue()

        self._run = None
        self._collecting = False
        self._ver = 0

    def stop(self):
        self._stop.set()

    def version(self):
        with self._lock:
            return self._ver

    def snapshot(self):
        with self._lock:
            r = self._run
            if r is None:
                return None
            return (r.mode, r.unit, list(r.t), list(r.ref), list(r.meas),
                    r.complete)

    def run(self):
        try:
            ser = serial.Serial(self.port, self.baud, timeout=0.2)
        except Exception as exc:                       # noqa: BLE001
            self.status.put(f"[ERRO] Nao abriu {self.port}: {exc}")
            return

        self.status.put(f"[OK] Conectado em {self.port} @ {self.baud}")
        with ser:
            while not self._stop.is_set():
                try:
                    raw = ser.readline()
                except Exception as exc:               # noqa: BLE001
                    self.status.put(f"[ERRO] Leitura: {exc}")
                    break
                if not raw:
                    continue
                line = raw.decode("ascii", errors="ignore").strip()
                if line:
                    self._parse(line)
        self.status.put("[--] Desconectado")

    def _parse(self, line):
        if line.startswith("#TEST_BEGIN"):
            kv = _kv(line)
            with self._lock:
                self._run = TestRun(mode=kv.get("mode", "?"),
                                    unit=kv.get("unit", "?"))
                self._collecting = True
                self._ver += 1
            self.status.put(f"[TEST] inicio mode={kv.get('mode','?')}")
            return

        if line.startswith("#TEST_END"):
            with self._lock:
                if self._run is not None:
                    self._run.complete = True
                self._collecting = False
                self._ver += 1
            self.status.put("[TEST] fim")
            return

        if self._collecting and ";" in line:
            parts = line.split(";")
            if len(parts) != 3:
                return
            try:
                t, r, m = int(parts[0]), int(parts[1]), int(parts[2])
            except ValueError:
                return
            with self._lock:
                self._run.t.append(t)
                self._run.ref.append(r)
                self._run.meas.append(m)
                self._ver += 1


def _kv(line):
    out = {}
    for tok in line.split():
        if "=" in tok:
            key, _, val = tok.partition("=")
            out[key] = val
    return out


def analyze(t, ref, meas):
    """Metricas dos degraus 0->+target e 0->-target (uma rodada por sequencia).

    Tempo de acomodacao: instante em que |medido| atinge (1-SETTLE_TOL) do alvo.
    Erro de regime: media e desvio de (medido - alvo) na ultima metade do plateau.
    """
    t = np.asarray(t, dtype=float)
    ref = np.asarray(ref, dtype=float)
    meas = np.asarray(meas, dtype=float)
    out = {}
    if t.size == 0:
        return out

    for name, target in (("pos", ref.max()), ("neg", ref.min())):
        if target == 0.0:
            continue
        idx = np.where(ref == target)[0]
        if idx.size == 0:
            continue
        brk = np.where(np.diff(idx) > 1)[0]
        if brk.size:
            idx = idx[:brk[0] + 1]                    # 1o plateau contiguo
        seg = np.arange(idx[0], idx[-1] + 1)
        t0, t1 = t[seg[0]], t[seg[-1]]

        thr = (1.0 - SETTLE_TOL) * target
        hit = seg[meas[seg] >= thr] if target > 0 else seg[meas[seg] <= thr]
        settle = (t[hit[0]] - t0) if hit.size else float("nan")

        if target > 0:
            ipk = seg[np.argmax(meas[seg])]
            over = meas[ipk] - target
        else:
            ipk = seg[np.argmin(meas[seg])]
            over = target - meas[ipk]

        win = seg[t[seg] >= (t1 - STEADY_HALF_MS)]
        err = meas[win] - target
        out[name] = {
            "target": target, "t0": t0, "t1": t1, "settle": settle,
            "overshoot": float(over),
            "overshoot_pct": float(over / abs(target) * 100.0),
            "peak_t": t[ipk], "peak_v": meas[ipk],
            "err_mean": float(err.mean()) if err.size else float("nan"),
            "err_std": float(err.std(ddof=1)) if err.size > 1 else 0.0,
        }
    return out


# --------------------------------------------------------------------------- #
#  Estilo de figura academico (IEEE), otimizado para P&B
# --------------------------------------------------------------------------- #
def _ieee_rc(mpl):
    mpl.rcParams.update({
        "font.family": "serif",
        "font.serif": ["Times New Roman", "DejaVu Serif"],
        "mathtext.fontset": "dejavuserif",
        "font.size": 8,
        "axes.labelsize": 8,
        "legend.fontsize": 7,
        "xtick.labelsize": 7,
        "ytick.labelsize": 7,
        "axes.linewidth": 0.6,
        "xtick.direction": "in",
        "ytick.direction": "in",
        "axes.grid": True,
        "grid.color": "0.85",
        "grid.linewidth": 0.4,
        "figure.autolayout": True,
    })


def _finish_ax(ax):
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    ax.tick_params(width=0.6, length=3)


# --------------------------------------------------------------------------- #
#  GUI
# --------------------------------------------------------------------------- #
class App:
    def __init__(self):
        self.worker = None
        self.last_ver = -1
        self.snap = None
        self.metrics = {}

    def refresh_ports(self):
        ports = [p.device for p in list_ports.comports()]
        dpg.configure_item("port_combo", items=ports)
        if ports and not dpg.get_value("port_combo"):
            dpg.set_value("port_combo", ports[0])

    def toggle_connect(self):
        if self.worker and self.worker.is_alive():
            self.worker.stop()
            self.worker.join(timeout=1.0)
            self.worker = None
            dpg.set_item_label("connect_btn", "Conectar")
            return

        port = dpg.get_value("port_combo")
        baud = dpg.get_value("baud_combo")
        if not port:
            self._log("[ERRO] Selecione uma porta")
            return
        self.last_ver = -1
        self.worker = SerialWorker(port, baud)
        self.worker.start()
        dpg.set_item_label("connect_btn", "Desconectar")

    def update_plot(self, snap):
        self.snap = snap
        mode, unit, t, ref, meas, complete = snap
        tf = [float(v) for v in t]
        dpg.set_value("ref_series", [tf, [float(v) for v in ref]])
        dpg.set_value("meas_series", [tf, [float(v) for v in meas]])
        dpg.configure_item("y_axis", label=GUI_YLABEL.get(unit, unit))
        dpg.fit_axis_data("x_axis")
        dpg.fit_axis_data("y_axis")
        try:
            self.metrics = analyze(t, ref, meas)
        except Exception:                               # noqa: BLE001
            self.metrics = {}
        self._update_stats()

    def _update_stats(self):
        if not self.snap:
            return
        mode, unit, t, ref, meas, complete = self.snap
        n = len(t)
        dur = t[-1] if t else 0
        m = self.metrics or {}
        p, ng = m.get("pos"), m.get("neg")

        def s_ms(d):
            if d and d["settle"] == d["settle"]:
                return f"{d['settle']:.0f} ms"
            return "--"

        def e_str(d):
            if d:
                return f"{d['err_mean']:.1f} ± {d['err_std']:.1f} {unit}"
            return "--"

        def o_str(d):
            return f"{d['overshoot_pct']:+.1f}%" if d else "--"

        dpg.set_value("stat_mode", f"Modo:          {mode}")
        dpg.set_value("stat_unit", f"Unidade:       {unit}")
        dpg.set_value("stat_n", f"Amostras:      {n}")
        dpg.set_value("stat_dur", f"Duração:       {dur} ms")
        dpg.set_value("stat_tset", f"t_acom (+/−):  {s_ms(p)} / {s_ms(ng)}")
        dpg.set_value("stat_over", f"Overshoot(+/−):{o_str(p)} / {o_str(ng)}")
        dpg.set_value("stat_errp", f"Erro reg (+):  {e_str(p)}")
        dpg.set_value("stat_errn", f"Erro reg (−):  {e_str(ng)}")
        dpg.set_value("stat_state",
                      f"Estado:        {'completo' if complete else 'coletando...'}")

    def export_csv(self):
        if not self.snap:
            self._log("[--] Nada para exportar")
            return
        mode, unit, t, ref, meas, _ = self.snap
        os.makedirs(OUTPUT_DIR, exist_ok=True)
        fname = os.path.join(OUTPUT_DIR, f"step_{mode}_{int(time.time())}.csv")
        with open(fname, "w", encoding="utf-8") as fp:
            fp.write(f"t_ms;ref_{unit};meas_{unit}\n")
            for i in range(len(t)):
                fp.write(f"{t[i]};{ref[i]};{meas[i]}\n")
        self._log(f"[OK] Salvo {os.path.basename(fname)} ({len(t)} amostras)")

    def export_figure(self):
        if not self.snap:
            self._log("[--] Nada para exportar")
            return
        try:
            import matplotlib
            matplotlib.use("Agg")
            import matplotlib.pyplot as plt
        except Exception as exc:                        # noqa: BLE001
            self._log(f"[ERRO] instale matplotlib: {exc}")
            return

        mode, unit, t, ref, meas, _ = self.snap
        _ieee_rc(matplotlib)
        ts = [v / 1000.0 for v in t]
        m = analyze(t, ref, meas)

        fig, ax = plt.subplots(figsize=(3.5, 2.2))
        for d in m.values():
            ax.axvspan((d["t1"] - STEADY_HALF_MS) / 1000.0, d["t1"] / 1000.0,
                       color="0.90", linewidth=0.0, zorder=0)
            if d["settle"] == d["settle"]:
                ax.axvline((d["t0"] + d["settle"]) / 1000.0, color="0.45",
                           linestyle=":", linewidth=0.7, zorder=3)
        ax.plot(ts, meas, color="black", linewidth=0.8, label="Medido")
        ax.plot(ts, ref, drawstyle="steps-post", color=REF_COLOR, linewidth=1.0,
                linestyle="--", label="Ref.", zorder=5)
        for k, d in m.items():
            sig = "+" if k == "pos" else "−"
            st = f"{d['settle']:.0f} ms" if d["settle"] == d["settle"] else "--"
            self._log(f"[{sig}] t_acom={st}  over={d['overshoot_pct']:+.1f}%  "
                      f"erro_reg={d['err_mean']:.1f}±{d['err_std']:.1f} {unit}")
        ax.set_xlabel("Tempo (s)")
        ax.set_ylabel(FIG_YLABEL.get(unit, unit))
        ax.legend(frameon=False, loc="best")
        _finish_ax(ax)
        os.makedirs(OUTPUT_DIR, exist_ok=True)
        base = os.path.join(OUTPUT_DIR, f"fig_step_{mode}_{int(time.time())}")
        fig.savefig(base + ".pdf")
        fig.savefig(base + ".png", dpi=300)
        plt.close(fig)
        self._log(f"[OK] Figura salva: {os.path.basename(base)} (.pdf/.png)")

    def _log(self, msg):
        old = dpg.get_value("log_text")
        lines = (old + "\n" + msg).splitlines()[-200:]
        dpg.set_value("log_text", "\n".join(lines))

    def poll(self):
        if not self.worker:
            return
        while not self.worker.status.empty():
            self._log(self.worker.status.get_nowait())
        v = self.worker.version()
        if v != self.last_ver:
            self.last_ver = v
            snap = self.worker.snapshot()
            if snap:
                self.update_plot(snap)


def build_gui(app):
    dpg.create_context()

    with dpg.window(tag="root"):
        with dpg.group(horizontal=True):
            dpg.add_combo([], tag="port_combo", width=140, label="Porta")
            dpg.add_button(label="Atualizar", callback=lambda: app.refresh_ports())
            dpg.add_combo(BAUD_OPTIONS, default_value=DEFAULT_BAUD,
                          tag="baud_combo", width=100, label="Baud")
            dpg.add_button(label="Conectar", tag="connect_btn",
                           callback=lambda: app.toggle_connect())
            dpg.add_spacer(width=20)
            dpg.add_button(label="Exportar CSV", callback=lambda: app.export_csv())
            dpg.add_button(label="Exportar figura",
                           callback=lambda: app.export_figure())

        with dpg.group(horizontal=True):
            with dpg.plot(label="Resposta ao degrau", height=-1, width=-320):
                dpg.add_plot_legend()
                dpg.add_plot_axis(dpg.mvXAxis, label="Tempo (ms)", tag="x_axis")
                with dpg.plot_axis(dpg.mvYAxis, label="Valor", tag="y_axis"):
                    dpg.add_line_series([], [], label="Referência",
                                        tag="ref_series")
                    dpg.add_line_series([], [], label="Medido", tag="meas_series")

            with dpg.child_window(width=310):
                dpg.add_text("Resumo", color=(120, 200, 255))
                dpg.add_separator()
                for tag in ("stat_mode", "stat_unit", "stat_n", "stat_dur",
                            "stat_tset", "stat_over", "stat_errp", "stat_errn",
                            "stat_state"):
                    dpg.add_text("", tag=tag)
                dpg.add_separator()
                dpg.add_text("Log", color=(120, 200, 255))
                with dpg.child_window(height=-1):
                    dpg.add_text("", tag="log_text", wrap=290)

    dpg.create_viewport(title="Ensaio dinâmico - Resposta ao degrau",
                        width=1100, height=620)
    dpg.setup_dearpygui()
    dpg.show_viewport()
    dpg.set_primary_window("root", True)
    app.refresh_ports()

    while dpg.is_dearpygui_running():
        app.poll()
        dpg.render_dearpygui_frame()

    if app.worker:
        app.worker.stop()
    dpg.destroy_context()


if __name__ == "__main__":
    build_gui(App())
