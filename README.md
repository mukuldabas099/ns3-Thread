# 🌐 Thread Mesh Network Simulation (NS-3)

## 📌 Overview

This project simulates a **Thread Mesh Network** using **NS-3** to analyze network performance under different conditions.

It evaluates key metrics like:

* 📊 Packet Delivery Ratio (PDR)
* 📉 Packet Loss
* ⏱️ Average Delay
* 🚀 Throughput (Network + Per Node)

The project also **automatically detects performance thresholds** where the network degrades from:

> EXCELLENT → GOOD → DEGRADED → POOR

---

## ⚙️ Features

* ✅ Thread mesh network simulation using NS-3
* ✅ Automated scenario execution via bash script
* ✅ Performance analysis across multiple parameters
* ✅ Automatic quality classification
* ✅ CSV result generation for further analysis

---

## 🧠 Network Quality Classification

The system classifies network performance based on:

| Quality   | Condition                    |
| --------- | ---------------------------- |
| EXCELLENT | PDR ≥ 95% AND Delay < 100 ms |
| GOOD      | PDR ≥ 85% AND Delay < 500 ms |
| DEGRADED  | PDR ≥ 60%                    |
| POOR      | PDR < 60%                    |

---

## 📁 Project Structure

```
.
├── thread-pdr-simulation.cc     # Main NS-3 simulation file
├── scratch-simulator.cc        # Additional simulation file
├── run_pdr_scenarios.sh        # Scenario automation script
├── CMakeLists.txt              # Build configuration
```

👉 The CMake file automatically detects `.cc` files and builds them as NS-3 scratch programs 

---

## 🚀 Scenarios Covered

The script runs **3 major experiments**:

### 1️⃣ Node Count Analysis

* Varies number of nodes
* Measures scalability of mesh network

### 2️⃣ Node Spacing (Distance)

* Tests effect of distance between nodes
* Shows communication range limits

### 3️⃣ Traffic Load (Packets per Node)

* Increases network load
* Evaluates congestion handling

👉 All scenarios are automated using:

* `run_pdr_scenarios.sh` 

---

## 🛠️ Requirements

* NS-3 (tested with ns-3.36 / ns-3.40)
* Linux environment (Ubuntu / Raspberry Pi OS)
* Bash shell

---

## ⚡ Setup & Run

### 1. Clone Repository

```bash
git clone https://github.com/your-username/your-repo-name.git
cd your-repo-name
```

### 2. Copy Simulation File to NS-3

```bash
cp thread-pdr-simulation.cc ~/ns-allinone-3.xx/ns-3.xx/scratch/
```

### 3. Run Scenarios

```bash
bash run_pdr_scenarios.sh
```

---

## 📊 Output Files

After execution, the following CSV files are generated:

| File                     | Description                    |
| ------------------------ | ------------------------------ |
| `results_nodes.csv`      | Performance vs number of nodes |
| `results_spacing.csv`    | Performance vs node distance   |
| `results_packets.csv`    | Performance vs traffic load    |
| `thread_pdr_results.csv` | Combined master log            |

---

## 📈 Key Insights

* Network performance drops as:

  * Node count increases
  * Distance increases
  * Traffic load increases

* The script automatically identifies:

  * Maximum stable network size
  * Maximum communication distance
  * Maximum traffic before degradation

---

## 🔥 Use Cases

* IoT network design (Thread / Zigbee)
* Wireless sensor network research
* Smart agriculture systems 🌱
* Performance benchmarking of mesh networks

---

## 👨‍💻 Author

**Mukul Dabas**

---

## 📜 License

This project is for educational and research purposes.
