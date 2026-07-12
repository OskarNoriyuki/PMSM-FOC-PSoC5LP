"""
plot_torque_static.py — Ensaio de torque estatico (Iq x torque)

Le torque_static.csv (delimitador ';', decimal ','), recalcula torque em
kgf.cm a partir das leituras da balanca (alavanca de 10 cm), obtem media e
desvio por corrente, e plota torque em funcao de Iq no estilo academico.

Conversao:  torque[kgf.cm] = massa[g]/1000 * bracoco[cm] = massa[g]/100  (braco 10 cm)

Requisitos:  pip install matplotlib numpy
Uso:         python plot_torque_static.py
"""

import os
import csv

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

CSV_IN = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                      "torque_static.csv")
OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                       "Generated_Torque")

LEVER_CM = 10.0        # braco da alavanca
FIT_MAX_A = 9.0        # regiao linear usada no ajuste (antes da instabilidade)
UNSTABLE_A = 17.0      # a partir daqui a leitura ficou instavel
REF_COLOR = "red"      # cor do ajuste linear


def _f(s):
    return float(s.replace(",", "."))


def load(path):
    Iq, taus = [], []
    with open(path, "r", encoding="utf-8") as fp:
        rd = csv.reader(fp, delimiter=";")
        next(rd)                                    # cabecalho
        for row in rd:
            if not row or not row[0]:
                continue
            Iq.append(_f(row[0]))
            grams = [_f(row[1]), _f(row[2]), _f(row[3])]
            taus.append([g / 100.0 for g in grams])  # g -> kgf.cm (braco 10 cm)
    Iq = np.array(Iq)
    taus = np.array(taus)
    mean = taus.mean(axis=1)
    std = taus.std(axis=1, ddof=1)
    return Iq, mean, std


def ieee_style():
    matplotlib.rcParams.update({
        "font.family": "serif",
        "font.serif": ["Times New Roman", "DejaVu Serif"],
        "mathtext.fontset": "dejavuserif",
        "font.size": 8, "axes.labelsize": 8, "legend.fontsize": 7,
        "xtick.labelsize": 7, "ytick.labelsize": 7,
        "axes.linewidth": 0.6, "xtick.direction": "in", "ytick.direction": "in",
        "axes.grid": True, "grid.color": "0.85", "grid.linewidth": 0.4,
        "figure.autolayout": True,
    })


def main():
    Iq, mean, std = load(CSV_IN)
    ieee_style()

    # ajuste linear (constante de torque) na regiao linear
    mask = Iq <= FIT_MAX_A
    kt, b = np.polyfit(Iq[mask], mean[mask], 1)

    fig, ax = plt.subplots(figsize=(3.5, 2.4))

    # regiao instavel
    ax.axvspan(UNSTABLE_A, Iq.max() + 0.5, color="0.90", linewidth=0)

    # reta de ajuste
    xline = np.array([0.0, Iq.max() + 0.5])
    ax.plot(xline, kt * xline + b, color=REF_COLOR, linewidth=1.0,
            linestyle="--", zorder=4,
            label=f"Ajuste: {kt:.3f} kgf$\\cdot$cm/A")

    # medidas com barra de desvio
    ax.errorbar(Iq, mean, yerr=std, fmt="o", color="black", markersize=3,
                elinewidth=0.7, capsize=2, capthick=0.7, zorder=5,
                label="Medido (média ± σ)")

    ax.set_xlabel(r"Corrente $I_q$ (A)")
    ax.set_ylabel(r"Torque (kgf$\cdot$cm)")
    ax.set_xlim(0, Iq.max() + 0.5)
    ax.set_ylim(0, None)
    ax.legend(frameon=False, loc="upper left")
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    ax.tick_params(width=0.6, length=3)

    os.makedirs(OUT_DIR, exist_ok=True)
    base = os.path.join(OUT_DIR, "fig_torque_static")
    fig.savefig(base + ".pdf")
    fig.savefig(base + ".png", dpi=300)
    plt.close(fig)
    print(f"kt = {kt:.4f} kgf.cm/A  (ajuste Iq <= {FIT_MAX_A} A)")
    print(f"figura salva em {base}.pdf / .png")


if __name__ == "__main__":
    main()
