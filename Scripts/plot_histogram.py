"""
plot_histogram.py — Visualizador do histograma de tempo de execucao (DutyMeas)

Le pela serial os blocos emitidos pelo firmware:

    #HIST_BEGIN res_us=1 bins=2000
      50: 0 0 0 0 1 0 0 58 554 2087
      60: 1159 138 2 0 1 0 0 0 0 0
      ...
    #HIST_END task=HFT wcet_us=168 overflow=0 samples=40000

Cada linha de dados traz o indice (us) do primeiro bin e 10 contagens.
A leitura roda em uma thread separada; a GUI (DearPyGui) so consome o
ultimo histograma completo.

Requisitos:  pip install pyserial dearpygui
Uso:         python plot_histogram.py
"""

import threading
import queue
import time
from dataclasses import dataclass, field

import serial
from serial.tools import list_ports
import dearpygui.dearpygui as dpg


BAUD_OPTIONS = ["9600", "115200", "230400", "460800", "921600"]
DEFAULT_BAUD = "460800"
DEFAULT_DEADLINE_US = 1000          # HFT @ 4 kHz -> 250 us ; LFT @ 1 kHz -> 1000 us
SHOW_DEADLINE_DEFAULT = False       # se False, centraliza o eixo x na media


@dataclass
class HistResult:
    task: str = "?"
    wcet_us: int = 0
    overflow: int = 0
    samples: int = 0
    res_us: int = 1
    nbins: int = 0                                # tamanho do buffer no firmware
    counts: list = field(default_factory=list)   # contagem por bin (indice = us)


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
        self._latest = None
        self._new = False
        self.status = queue.Queue()

        # estado do parser
        self._acc = {}
        self._res_us = 1
        self._nbins = 0
        self._collecting = False

    # -- API consumida pela GUI ------------------------------------------- #
    def stop(self):
        self._stop.set()

    def pop_result(self):
        with self._lock:
            if self._new:
                self._new = False
                return self._latest
        return None

    # -- thread ----------------------------------------------------------- #
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

    # -- parser ----------------------------------------------------------- #
    def _parse(self, line):
        if line.startswith("#HIST_BEGIN"):
            kv = _kv(line)
            self._acc = {}
            self._res_us = int(kv.get("res_us", 1))
            self._nbins = int(kv.get("bins", 0))
            self._collecting = True
            return

        if line.startswith("#HIST_END"):
            if not self._collecting:
                return
            self._collecting = False
            kv = _kv(line)
            n = (max(self._acc) + 1) if self._acc else 0
            counts = [self._acc.get(i, 0) for i in range(n)]
            res = HistResult(
                task=kv.get("task", "?"),
                wcet_us=int(kv.get("wcet_us", 0)),
                overflow=int(kv.get("overflow", 0)),
                samples=int(kv.get("samples", 0)),
                res_us=self._res_us,
                nbins=self._nbins,
                counts=counts,
            )
            with self._lock:
                self._latest = res
                self._new = True
            self.status.put(
                f"[HIST] task={res.task} wcet={res.wcet_us}us "
                f"samples={res.samples} overflow={res.overflow}"
            )
            return

        if self._collecting and ":" in line:
            head, _, tail = line.partition(":")
            try:
                base = int(head)
                for k, tok in enumerate(tail.split()):
                    self._acc[base + k] = int(tok)
            except ValueError:
                pass


def _kv(line):
    """Extrai tokens 'chave=valor' de uma linha de metadados."""
    out = {}
    for tok in line.split():
        if "=" in tok:
            key, _, val = tok.partition("=")
            out[key] = val
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


def _save(fig, name, plt):
    fig.savefig(name + ".pdf")
    fig.savefig(name + ".png", dpi=300)
    plt.close(fig)


# --------------------------------------------------------------------------- #
#  GUI
# --------------------------------------------------------------------------- #
class App:
    def __init__(self):
        self.worker = None
        self.result = None

    # -- serial control --------------------------------------------------- #
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
        self.worker = SerialWorker(port, baud)
        self.worker.start()
        dpg.set_item_label("connect_btn", "Desconectar")

    # -- plot ------------------------------------------------------------- #
    def update_plot(self, res):
        self.result = res
        x = list(range(len(res.counts)))
        y = [float(c) for c in res.counts]
        dpg.set_value("hist_series", [x, y])
        dpg.configure_item("hist_series", label=f"{res.task} (us)")

        ymax = max(y) if y else 1.0
        dpg.set_value("wcet_series", [[res.wcet_us, res.wcet_us], [0.0, ymax * 1.05]])

        self._draw_deadline(ymax)
        dpg.fit_axis_data("x_axis")
        dpg.fit_axis_data("y_axis")
        self._update_stats()

    def _draw_deadline(self, ymax=None):
        if not dpg.get_value("show_deadline"):
            dpg.set_value("deadline_series", [[], []])
            return
        d = dpg.get_value("deadline_input")
        if ymax is None:
            ymax = max((max(self.result.counts) if self.result and self.result.counts
                        else 1), 1)
        dpg.set_value("deadline_series", [[d, d], [0.0, float(ymax) * 1.05]])

    def _update_stats(self):
        res = self.result
        if not res:
            return
        d = dpg.get_value("deadline_input")
        total = res.samples if res.samples else sum(res.counts)
        weighted = sum(i * c for i, c in enumerate(res.counts))
        mean = (weighted / sum(res.counts)) if sum(res.counts) else 0.0
        hits = sum(res.counts[:d]) if d < len(res.counts) else sum(res.counts)
        hit_pct = (100.0 * hits / total) if total else 0.0
        margin = d - res.wcet_us

        dpg.set_value("stat_task", f"Task medida:     {res.task}")
        dpg.set_value("stat_samples", f"Amostras:        {res.samples}")
        dpg.set_value("stat_mean", f"Media:           {mean:.1f} us")
        dpg.set_value("stat_wcet", f"WCET:            {res.wcet_us} us")
        dpg.set_value("stat_deadline", f"Deadline:        {d} us")
        dpg.set_value(
            "stat_margin",
            f"Folga (WCET):    {margin} us  ({'OK' if margin >= 0 else 'ESTOUROU'})",
        )
        dpg.set_value("stat_hits", f"Dentro do prazo: {hit_pct:.3f} %")
        dpg.set_value("stat_overflow", f"Overflow (>buf): {res.overflow}")

    # -- misc ------------------------------------------------------------- #
    def export_csv(self):
        if not self.result:
            self._log("[--] Nada para exportar")
            return
        res = self.result
        n = res.nbins if res.nbins > 0 else len(res.counts)
        fname = f"hist_{res.task}_{int(time.time())}.csv"
        with open(fname, "w", encoding="utf-8") as fp:
            fp.write("us;count\n")               # ';' evita colisao com decimal pt-BR
            for i in range(n):
                c = res.counts[i] if i < len(res.counts) else 0
                fp.write(f"{i};{c}\n")
        self._log(f"[OK] Salvo {fname} ({n} bins)")

    def export_figure(self):
        res = self.result
        if not res:
            self._log("[--] Nada para exportar")
            return
        try:
            import numpy as np
            import matplotlib
            matplotlib.use("Agg")
            import matplotlib.pyplot as plt
        except Exception as exc:                    # noqa: BLE001
            self._log(f"[ERRO] instale matplotlib/numpy: {exc}")
            return

        _ieee_rc(matplotlib)
        d = float(dpg.get_value("deadline_input"))
        counts = np.asarray(res.counts, dtype=float)
        x = np.arange(counts.size)
        show_dl = bool(dpg.get_value("show_deadline"))
        s = counts.sum()
        mean = float((x @ counts) / s) if s else 0.0
        nz = np.nonzero(counts)[0]
        lo = float(nz[0]) if nz.size else 0.0
        hi = float(nz[-1]) if nz.size else float(counts.size)
        if show_dl:
            xlo, xhi = 0.0, max(d, res.wcet_us) * 1.20
        else:
            half = max(mean - lo, hi - mean, 5.0) * 1.30
            xlo, xhi = max(0.0, mean - half), mean + half
        stamp = int(time.time())
        base = f"fig_{res.task}_{stamp}"

        # ---- histograma ------------------------------------------------- #
        fig, ax = plt.subplots(figsize=(3.5, 2.2))
        ax.fill_between(x, counts, step="mid", color="0.82", linewidth=0.0)
        ax.plot(x, counts, drawstyle="steps-mid", color="black", linewidth=0.8)
        ymax = (counts.max() * 1.15) if counts.size else 1.0
        ax.axvline(res.wcet_us, color="black", linestyle=":", linewidth=0.8, zorder=5)
        ax.text(res.wcet_us, ymax * 0.90, " WCET", va="top", ha="left", fontsize=6,
                clip_on=False, zorder=5)
        if show_dl:
            ax.axvline(d, color="black", linestyle="--", linewidth=0.8, zorder=5)
            ax.text(d, ymax * 0.97, " deadline", va="top", ha="left", fontsize=6,
                    clip_on=False, zorder=5)
        ax.set_xlim(xlo, xhi)
        ax.set_ylim(0.0, ymax)
        ax.set_xlabel("Tempo de execução (µs)")
        ax.set_ylabel("Ocorrências")
        _finish_ax(ax)
        _save(fig, base + "_hist", plt)

        # ---- ECDF (bom para verificar folga de deadline) ---------------- #
        total = counts.sum()
        ecdf = (np.cumsum(counts) / total * 100.0) if total else counts
        fig, ax = plt.subplots(figsize=(3.5, 2.2))
        ax.plot(x, ecdf, drawstyle="steps-post", color="black", linewidth=0.9)
        if show_dl:
            ax.axvline(d, color="black", linestyle="--", linewidth=0.8, zorder=5)
        ax.axhline(100.0, color="0.6", linestyle="-", linewidth=0.4)
        ax.set_xlim(xlo, xhi)
        ax.set_ylim(0.0, 101.0)
        ax.set_xlabel("Tempo de execução (µs)")
        ax.set_ylabel("Prob. acumulada (%)")
        _finish_ax(ax)
        _save(fig, base + "_ecdf", plt)

        self._log(f"[OK] Figuras salvas: {base}_hist / {base}_ecdf (.pdf/.png)")

    def _log(self, msg):
        old = dpg.get_value("log_text")
        lines = (old + "\n" + msg).splitlines()[-200:]
        dpg.set_value("log_text", "\n".join(lines))

    # -- loop ------------------------------------------------------------- #
    def poll(self):
        if self.worker:
            while not self.worker.status.empty():
                self._log(self.worker.status.get_nowait())
            res = self.worker.pop_result()
            if res:
                self.update_plot(res)


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
            dpg.add_input_int(label="Deadline (us)", tag="deadline_input",
                              default_value=DEFAULT_DEADLINE_US, width=120,
                              step=10, callback=lambda: (app._draw_deadline(),
                                                         app._update_stats()))
            dpg.add_checkbox(label="Deadline", tag="show_deadline",
                             default_value=SHOW_DEADLINE_DEFAULT,
                             callback=lambda: (app._draw_deadline(),
                                               app._update_stats()))
            dpg.add_button(label="Exportar CSV", callback=lambda: app.export_csv())
            dpg.add_button(label="Exportar figura", callback=lambda: app.export_figure())

        with dpg.group(horizontal=True):
            with dpg.plot(label="Histograma de tempo de execucao",
                          height=-1, width=-320):
                dpg.add_plot_legend()
                dpg.add_plot_axis(dpg.mvXAxis, label="Tempo de execucao (us)",
                                  tag="x_axis")
                with dpg.plot_axis(dpg.mvYAxis, label="Ocorrencias", tag="y_axis"):
                    dpg.add_bar_series([], [], label="hist", tag="hist_series",
                                       weight=1.0)
                    dpg.add_line_series([], [], label="deadline",
                                        tag="deadline_series")
                    dpg.add_line_series([], [], label="WCET", tag="wcet_series")

            with dpg.child_window(width=310):
                dpg.add_text("Resumo", color=(120, 200, 255))
                dpg.add_separator()
                for tag in ("stat_task", "stat_samples", "stat_mean", "stat_wcet",
                            "stat_deadline", "stat_margin", "stat_hits",
                            "stat_overflow"):
                    dpg.add_text("", tag=tag)
                dpg.add_separator()
                dpg.add_text("Log", color=(120, 200, 255))
                with dpg.child_window(height=-1):
                    dpg.add_text("", tag="log_text", wrap=290)

    dpg.create_viewport(title="DutyMeas - Histograma", width=1100, height=620)
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
