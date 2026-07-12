"""
plot_vq_speed.py — Acionamento por tensão constante (malha aberta)

Velocidade em regime x tensão aplicada (sem controle de corrente). O eixo é a
tensão eficaz entre fases V_LL, obtida do comando Vq pela relação do modulador
(V_LL,rms = sqrt(2/3)*Vq). Calcula média, desvio, ajuste linear e a
constante de velocidade Kv, e plota no estilo acadêmico.

Requisitos:  pip install matplotlib numpy
Uso:         python plot_vq_speed.py
"""

import os

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                       "Generated_Vq")
REF_COLOR = "red"
VLL_PER_VQ = (2.0 / 3.0) ** 0.5   # V_LL,rms por unidade de Vq: sqrt(2/3) (modulador SVPWM)

# amostras de velocidade [rpm] por comando Vq
DATA = {
    1: [377, 353, 351, 322, 366, 351, 336, 367, 351, 354, 354, 352, 355,
        369, 354, 352, 355, 337, 352, 352, 351, 364, 379, 351, 347],
    2: [738, 707, 708, 707, 718, 703, 703, 703, 703, 697, 699, 706, 716,
        723, 718, 718, 707, 707, 718, 710, 703, 703, 710, 703],
    3: [1021, 1019, 1039, 1057, 1067, 1085, 1047, 1039, 1050, 1038, 1035,
        1035, 1050, 1040, 1054, 1062, 1054, 1037, 1046, 1037, 1016, 1035, 1038],
}


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
    Vq = np.array(sorted(DATA), dtype=float)
    Vll = VLL_PER_VQ * Vq                               # Vq -> Vrms linha-linha
    mean = np.array([np.mean(DATA[int(v)]) for v in Vq])
    std = np.array([np.std(DATA[int(v)], ddof=1) for v in Vq])

    a, b = np.polyfit(Vll, mean, 1)                     # ajuste com intercepto
    res = mean - (a * Vll + b)
    r2 = 1.0 - res @ res / ((mean - mean.mean()) @ (mean - mean.mean()))
    kv = (Vll @ mean) / (Vll @ Vll)                     # ajuste pela origem

    ieee_style()
    fig, ax = plt.subplots(figsize=(3.5, 2.4))
    xline = np.array([0.0, Vll.max() + 0.3])
    ax.plot(xline, a * xline + b, color=REF_COLOR, linewidth=1.0, linestyle="--",
            zorder=3, label="Ajuste: %.0f rpm/V$_{\\mathrm{rms}}$" % a)
    ax.errorbar(Vll, mean, yerr=std, fmt="o", color="black", markersize=3,
                elinewidth=0.7, capsize=2, capthick=0.7, zorder=5,
                label="Medido (média ± σ)")

    ax.set_xlabel(r"Tensão de linha $V_{LL}$ (V$_{\mathrm{rms}}$)")
    ax.set_ylabel("Velocidade (rpm)")
    ax.set_xlim(0, Vll.max() + 0.3)
    ax.set_ylim(0, None)
    ax.legend(frameon=False, loc="upper left")
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    ax.tick_params(width=0.6, length=3)

    os.makedirs(OUT_DIR, exist_ok=True)
    base = os.path.join(OUT_DIR, "fig_vq_speed")
    fig.savefig(base + ".pdf")
    fig.savefig(base + ".png", dpi=300)
    plt.close(fig)
    print("ajuste: n = %.2f Vll + %.2f  (R2=%.5f)" % (a, b, r2))
    print("Kv (origem) = %.1f rpm/Vrms(linha)" % kv)
    print("figura salva em %s.pdf / .png" % base)


if __name__ == "__main__":
    main()
