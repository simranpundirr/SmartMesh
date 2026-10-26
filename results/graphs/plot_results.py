import xml.etree.ElementTree as ET
import matplotlib.pyplot as plt
from pathlib import Path

OUT = Path("results/graphs")
SCENARIOS = ["light", "moderate", "heavy"]
PROTOCOLS = {
    "SmartMesh": "results/traffic/{}-flow-monitor.xml",
    "AODV": "results/aodv-traffic/{}-flow-monitor.xml",
}

def read_metrics(path):
    root = ET.parse(path).getroot()
    stats = {
        f.get("flowId"): f.attrib
        for f in root.find("FlowStats").findall("Flow")
    }
    classifier = root.find("Ipv4FlowClassifier")

    for flow in classifier.findall("Flow"):
        if flow.get("destinationPort") != "9000":
            continue
        s = stats[flow.get("flowId")]
        tx, rx = int(s["txPackets"]), int(s["rxPackets"])

        def ns(key):
            return float(s[key].replace("+", "").replace("ns", ""))

        duration = (ns("timeLastRxPacket") -
                    ns("timeFirstRxPacket")) / 1e9

        return {
            "pdr": 100 * rx / tx if tx else 0,
            "delay": ns("delaySum") / rx / 1e6 if rx else 0,
            "jitter": ns("jitterSum") / (rx - 1) / 1e6 if rx > 1 else 0,
            "throughput": int(s["rxBytes"]) * 8 / duration / 1000
                          if duration > 0 else 0,
            "tx": tx,
            "rx": rx,
        }
    raise ValueError(f"No UDP flow to port 9000 found in {path}")

data = {
    protocol: [read_metrics(pattern.format(s)) for s in SCENARIOS]
    for protocol, pattern in PROTOCOLS.items()
}

plots = [
    ("delay", "Average End-to-End Delay (ms)", "delay_comparison.png"),
    ("jitter", "Average Jitter (ms)", "jitter_comparison.png"),
    ("throughput", "Throughput (kb/s)", "throughput_comparison.png"),
    ("pdr", "Packet Delivery Ratio (%)", "pdr_comparison.png"),
]

for metric, ylabel, filename in plots:
    fig, ax = plt.subplots(figsize=(8, 5))
    x = list(range(len(SCENARIOS)))
    width = 0.34

    for offset, (protocol, results) in zip(
        [-width / 2, width / 2], data.items()
    ):
        values = [r[metric] for r in results]
        ax.bar([i + offset for i in x], values, width, label=protocol)

    ax.set_xticks(x)
    ax.set_xticklabels(["Light", "Moderate", "Heavy*"])
    ax.set_xlabel("Traffic Scenario")
    ax.set_ylabel(ylabel)
    ax.set_title(ylabel + " — SmartMesh vs AODV")
    ax.legend()
    ax.grid(axis="y", linestyle="--", alpha=0.35)
    fig.tight_layout()
    fig.savefig(OUT / filename, dpi=200)
    plt.close(fig)
    print(f"Saved {OUT / filename}")

print("\nHeavy scenario reached the 1,000-packet limit.")
print("Graphs use the existing FlowMonitor XML results.")
