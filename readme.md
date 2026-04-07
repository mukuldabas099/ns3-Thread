# Thread Mesh Network — PDR & Throughput Simulation
### NS-3 Based IEEE 802.15.4 / Thread Protocol Evaluation Framework

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [Background & Theory](#2-background--theory)
   - 2.1 [Thread Protocol Stack](#21-thread-protocol-stack)
   - 2.2 [IEEE 802.15.4 PHY/MAC Layer](#22-ieee-802154-phymac-layer)
   - 2.3 [6LoWPAN Adaptation Layer](#23-6lowpan-adaptation-layer)
   - 2.4 [AODV Routing Protocol](#24-aodv-routing-protocol)
3. [Key Performance Metrics — Definitions & Formulas](#3-key-performance-metrics--definitions--formulas)
   - 3.1 [Packet Delivery Ratio (PDR)](#31-packet-delivery-ratio-pdr)
   - 3.2 [Packet Loss Rate (PLR)](#32-packet-loss-rate-plr)
   - 3.3 [Average End-to-End Delay](#33-average-end-to-end-delay)
   - 3.4 [Network Throughput](#34-network-throughput)
   - 3.5 [Per-Node Throughput](#35-per-node-throughput)
4. [Quality Classification System](#4-quality-classification-system)
5. [Simulation Architecture](#5-simulation-architecture)
   - 5.1 [Network Topology](#51-network-topology)
   - 5.2 [Node Placement Models](#52-node-placement-models)
   - 5.3 [Traffic Model — OnOff Application](#53-traffic-model--onoff-application)
   - 5.4 [Data Rate Calculation](#54-data-rate-calculation)
6. [Measurement Methodology — Dual Counting](#6-measurement-methodology--dual-counting)
7. [Simulation Scenarios](#7-simulation-scenarios)
   - 7.1 [Scenario 1 — Node Count Sweep](#71-scenario-1--node-count-sweep)
   - 7.2 [Scenario 2 — Node Spacing Sweep](#72-scenario-2--node-spacing-sweep)
   - 7.3 [Scenario 3 — Packet Load Sweep](#73-scenario-3--packet-load-sweep)
8. [Threshold Detection Algorithm](#8-threshold-detection-algorithm)
9. [File Structure & Outputs](#9-file-structure--outputs)
10. [Installation & Prerequisites](#10-installation--prerequisites)
11. [How to Run](#11-how-to-run)
12. [Understanding the Output](#12-understanding-the-output)
13. [Worked Example with Sample Numbers](#13-worked-example-with-sample-numbers)
14. [Glossary](#14-glossary)

---

## 1. Project Overview

This project is a **network simulation framework** built on top of [NS-3](https://www.nsnam.org/) (Network Simulator 3) that evaluates the performance of a **Thread mesh network** — a low-power wireless protocol widely used in IoT and smart home devices (e.g., Google Nest, Apple HomePod, Samsung SmartThings).

The simulation answers three practical engineering questions:

> 1. **How many nodes** can a Thread mesh support before quality degrades?
> 2. **How far apart** can nodes be placed before the mesh starts failing?
> 3. **How much traffic** can the mesh carry before it becomes congested?

The framework runs automated parameter sweeps across all three questions and **automatically identifies the exact threshold** where network quality transitions between quality tiers.

### Files in this Project

| File | Purpose |
|---|---|
| `thread-pdr-simulation.cc` | NS-3 C++ simulation program (the core engine) |
| `run_scenarios.sh` | Bash orchestration script that runs all three sweeps |
| `results_nodes.csv` | Output data — Scenario 1 |
| `results_spacing.csv` | Output data — Scenario 2 |
| `results_packets.csv` | Output data — Scenario 3 |
| `thread_pdr_results.csv` | Master log of all individual simulation runs |
| `thread_pdr_flowmonitor.xml` | Raw NS-3 FlowMonitor export (XML) |

---

## 2. Background & Theory

### 2.1 Thread Protocol Stack

Thread is a **mesh networking protocol** designed for constrained IoT devices. It operates using a layered stack:

```
+---------------------------+
|   Application Layer       |   (e.g., CoAP, MQTT over UDP)
+---------------------------+
|   UDP / TCP               |   Transport Layer
+---------------------------+
|   IPv6                    |   Network Layer
+---------------------------+
|   6LoWPAN                 |   Adaptation Layer (header compression)
+---------------------------+
|   IEEE 802.15.4 MAC       |   Medium Access Control
+---------------------------+
|   IEEE 802.15.4 PHY       |   Physical Layer (2.4 GHz radio)
+---------------------------+
```

Key properties of Thread:
- Operates on the **2.4 GHz ISM band** (same as Wi-Fi and Bluetooth)
- Maximum raw data rate: **250 kbps** per IEEE 802.15.4
- Designed for small payloads (tens to hundreds of bytes per packet)
- Self-healing mesh — routes re-form automatically when a node fails
- Each network has exactly one **Border Router** that bridges to an IP backbone

### 2.2 IEEE 802.15.4 PHY/MAC Layer

The physical layer defines the radio characteristics:

| Parameter | Value |
|---|---|
| Frequency Band | 2.4 GHz (global) |
| Channels | 16 channels (channel 11–26) |
| Channel Spacing | 5 MHz |
| Raw Bit Rate | 250 kbps |
| Modulation | O-QPSK with DSSS |
| Max PSDU (payload) | 127 bytes |
| Typical range (indoor) | 10–30 m per hop |

**CSMA-CA (Carrier Sense Multiple Access with Collision Avoidance)** is the MAC access mechanism. Before transmitting, a node:
1. Senses the channel — if busy, waits a random backoff period
2. Retries up to a configurable number of times (default: 3)
3. If still unable to transmit, the packet is dropped → contributing to packet loss

The **link budget** determines how far two nodes can communicate:

```
Received Power (dBm) = Transmit Power (dBm) + Antenna Gain (dBi) - Path Loss (dB)
```

For free-space path loss between two nodes separated by distance `d` (in metres) at frequency `f` (in Hz):

```
Path Loss (dB) = 20·log₁₀(d) + 20·log₁₀(f) + 20·log₁₀(4π/c)
               = 20·log₁₀(d) + 20·log₁₀(f) - 147.55
```

Where `c = 3 × 10⁸ m/s`. At 2.4 GHz and d = 30 m:

```
Path Loss = 20·log₁₀(30) + 20·log₁₀(2.4×10⁹) - 147.55
          = 29.54 + 187.60 - 147.55
          = 69.59 dB
```

This is why `nodeSpacing` beyond ~50–80 m causes rapid PDR degradation in this simulation.

### 2.3 6LoWPAN Adaptation Layer

IPv6 headers are 40 bytes long — too large for an IEEE 802.15.4 frame (max 127 bytes payload). **6LoWPAN** solves this by:

- **Header compression**: Reduces the 40-byte IPv6 header to as few as 2 bytes using context-based compression
- **Fragmentation**: Splits large IPv6 packets across multiple 802.15.4 frames
- **Reassembly**: Reconstructs the IPv6 packet at the destination

In this simulation, `ForceEtherType = true` is set to ensure 6LoWPAN correctly marks device types for the NS-3 stack.

### 2.4 AODV Routing Protocol

**AODV (Ad-hoc On-Demand Distance Vector)** is the routing protocol used here. It is reactive — it only discovers routes when a node actually needs to send data.

**Route Discovery Process:**
1. Source node floods a **Route Request (RREQ)** broadcast
2. Intermediate nodes rebroadcast RREQ while recording the reverse path
3. Destination (Border Router) sends a **Route Reply (RREP)** back along the reverse path
4. Source begins sending data along the established route
5. If a link breaks, a **Route Error (RERR)** propagates and new discovery begins

**AODV overhead** increases with node count, which is one of the reasons PDR degrades as `totalNodes` increases — more RREQ/RREP control traffic competes with data traffic for the shared 250 kbps channel.

---

## 3. Key Performance Metrics — Definitions & Formulas

### 3.1 Packet Delivery Ratio (PDR)

PDR is the primary health metric. It measures what fraction of all sent packets successfully reached the Border Router.

**Formula:**

```
PDR (%) = (Packets Successfully Received / Packets Transmitted) × 100
```

More formally, if:
- `N_sent` = total packets injected by all mesh nodes into the network
- `N_recv` = total packets received at the Border Router

Then:

```
PDR = (N_recv / N_sent) × 100 %
```

This simulation computes PDR using **two independent methods** (see Section 6):

**Method A — Callback PDR:**
```
PDR_CB = (g_totalReceived / g_totalSent) × 100 %
```

**Method B — FlowMonitor PDR:**
```
PDR_FM = (Σ fmRecv_i / Σ fmSent_i) × 100 %
         for all flows i ∈ {1, ..., nMesh}
```

The FlowMonitor PDR (`PDR_FM`) is used for the quality classification because it accounts for all packets at the IP layer, including AODV control traffic overhead effects.

### 3.2 Packet Loss Rate (PLR)

PLR is the complement of PDR:

```
PLR (%) = 100 - PDR (%)
        = (N_lost / N_sent) × 100 %
```

Where `N_lost = N_sent - N_recv`.

Packets can be lost due to:
- **Radio collision** — two nodes transmit simultaneously → MAC drops the frame
- **Out of range** — nodes too far apart, signal below receiver sensitivity
- **Buffer overflow** — MAC queue full under heavy load
- **Route failure** — AODV has not yet found or re-established a path
- **Channel congestion** — too many nodes competing for the shared 250 kbps channel

### 3.3 Average End-to-End Delay

Delay is measured **per flow** and then averaged across all flows:

```
Delay_i (ms) = delaySum_i / rxPackets_i        [for flow i]

AvgDelay (ms) = (1/F) × Σ Delay_i
                for i = 1 to F (total number of flows)
```

Where:
- `delaySum_i` = cumulative sum of all individual packet delays in flow `i`, recorded by FlowMonitor
- `rxPackets_i` = number of successfully received packets in flow `i`
- `F` = total number of active flows (= number of mesh nodes, one flow per source)

The per-packet delay includes:
- **Queuing delay** — time spent in the MAC transmit queue
- **Transmission delay** — time to put the packet on the wire = `packetSize / dataRate`
- **Propagation delay** — negligible for distances < 100 m (`d/c ≈ 0.33 μs` for 100 m)
- **Processing delay** — MAC backoff, retransmission waiting time
- **Multi-hop forwarding delay** — accumulated at each relay node

**Transmission delay example** (64-byte packet at 250 kbps):
```
Tx delay = (64 bytes × 8 bits/byte) / (250,000 bits/s)
         = 512 bits / 250,000 bps
         = 2.048 ms per hop
```

For a 3-hop path this contributes ~6 ms of transmission delay alone.

### 3.4 Network Throughput

Net throughput is the aggregate data successfully delivered to the Border Router:

```
NetThroughput (kbps) = (TotalRxBytes × 8) / (SimTime × 1000)
```

Where:
- `TotalRxBytes` = Σ rxBytes across all flows (in bytes)
- `SimTime` = total simulation duration in seconds
- Dividing by 1000 converts bps → kbps

**Example:** If 200,000 bytes are received over 60 seconds:
```
NetThroughput = (200,000 × 8) / (60 × 1000)
              = 1,600,000 / 60,000
              = 26.67 kbps
```

Note: Maximum theoretical throughput with `nMesh` nodes each sending at `packetSize × 8 / packetInterval` bps on a shared 250 kbps channel:

```
OfferedLoad (kbps) = nMesh × packetSize × 8 / (packetInterval × 1000)
```

With 10 mesh nodes, 64-byte packets at 1-second intervals:
```
OfferedLoad = 10 × 64 × 8 / (1.0 × 1000) = 5.12 kbps
```

This is well below 250 kbps — congestion in this simulation arises primarily from **AODV routing overhead** and **CSMA-CA collisions**, not raw bandwidth exhaustion.

### 3.5 Per-Node Throughput

Derived from the net throughput, divided evenly across mesh sources:

```
PerNodeThroughput (kbps) = NetThroughput / nMesh
```

This metric shows the **fair share** each node receives. In a well-functioning mesh, this should remain stable as more nodes are added (until congestion onset).

---

## 4. Quality Classification System

The simulation uses **four quality tiers** based on IEEE 802.15.4 and Thread mesh industry benchmarks. These thresholds are fixed constants in the code and are not user-configurable — they reflect real engineering standards.

### Tier Table

| Label | PDR Condition | Delay Condition | Meaning |
|---|---|---|---|
| **EXCELLENT** | PDR ≥ 95% | AND Delay < 100 ms | Mesh at full capacity, optimal operation |
| **GOOD** | PDR ≥ 85% | AND Delay < 500 ms | Stable and usable, minor degradation |
| **DEGRADED** | PDR ≥ 60% | (any delay) | Significant packet loss, approaching failure |
| **POOR** | PDR < 60% | (any delay) | Severe failure, mesh barely functional |

### Classification Logic (from `ClassifyNetwork()`)

```
IF (PDR_FM >= 95.0) AND (AvgDelay < 100.0 ms):
    label = "EXCELLENT"

ELSE IF (PDR_FM >= 85.0) AND (AvgDelay < 500.0 ms):
    label = "GOOD"

ELSE IF (PDR_FM >= 60.0):
    label = "DEGRADED"

ELSE:
    label = "POOR"
```

### Rationale for Thresholds

| Threshold | Source / Rationale |
|---|---|
| PDR ≥ 95% for EXCELLENT | Thread specification requires ≥ 95% PDR for certified mesh operation |
| Delay < 100 ms for EXCELLENT | Human-perceptible control latency limit for IoT actuators (e.g., light switches) |
| PDR ≥ 85% for GOOD | Standard IoT sensor networks tolerate up to 15% loss before application impact |
| Delay < 500 ms for GOOD | Maximum one-way delay for real-time sensor reporting per IETF RFC 5405 |
| PDR ≥ 60% for DEGRADED | Below 60% PDR, reliable TCP/CoAP session establishment fails |
| PDR < 60% → POOR | Network is operationally useless for IoT applications |

---

## 5. Simulation Architecture

### 5.1 Network Topology

```
[Mesh Node 1] ──┐
[Mesh Node 2] ──┤
[Mesh Node 3] ──┼──► [Border Router]  ◄── (Sink / Destination)
      ...        │       Node 0
[Mesh Node N] ──┘
```

- **Node 0**: Border Router — acts as the packet sink (destination). All traffic flows toward it.
- **Nodes 1 to (totalNodes-1)**: Mesh nodes — each is a UDP traffic source sending to the Border Router.
- All nodes share a single **PAN (Personal Area Network)** on the same IEEE 802.15.4 channel (PAN ID = 0).
- All traffic is **unicast UDP** from each mesh node → Border Router.
- There is **one UDP flow per mesh node** → `nMesh` total flows.

### 5.2 Node Placement Models

Two placement modes are available, selected by the `nodeSpacing` parameter:

**Mode A — Grid Layout** (`nodeSpacing > 0`)

```
BR (0,0) ── Mesh1 (d,0) ── Mesh2 (2d,0) ── Mesh3 (3d,0) ── ...
            |<----d m---->|
```

Nodes are placed on a **1-dimensional line** along the X axis. The Border Router is at the origin `(0, 0, 0)` and each mesh node `i` is at `((i+1)×d, 0, 0)` where `d = nodeSpacing`.

The distance from mesh node `i` to the Border Router is:
```
distance_i = (i + 1) × nodeSpacing   [metres]
```

Nodes far from the Border Router must relay through intermediate nodes (multi-hop routing via AODV).

**Mode B — Random Layout** (`nodeSpacing = 0`)

```
  ┌─────────────── areaSize ───────────────┐
  │          *  Mesh3                      │
  │  Mesh1 *                               │
  │              [BR]  ← centre            │
  │     Mesh4 *      * Mesh2               │
  └────────────────────────────────────────┘
```

The Border Router is placed at the **centre** `(areaSize/2, areaSize/2, 0)`. Mesh nodes are placed uniformly at random within the `areaSize × areaSize` square. Positions are drawn from:
```
X ~ Uniform(0, areaSize)
Y ~ Uniform(0, areaSize)
```

### 5.3 Traffic Model — OnOff Application

Each mesh node uses NS-3's **OnOffApplication** in always-on mode:
- `OnTime  = Constant(1)` → always transmitting
- `OffTime = Constant(0)` → never pausing
- Source starts at `t = 1.0 + i × 0.1` seconds (staggered by 100 ms per node)
- Source stops at `t = simTime - 1.0` seconds

The **staggered start** (`i × 0.1 s`) prevents a thundering-herd startup where all nodes attempt AODV route discovery simultaneously.

**MaxBytes mode** (when `nPackets > 0`):
```
MaxBytes = nPackets × packetSize   [bytes]
```
The application stops automatically after sending exactly this many bytes, regardless of `simTime`.

### 5.4 Data Rate Calculation

The OnOff data rate is set to exactly one packet per `packetInterval` seconds:

```
DataRate (bps) = packetSize × 8 / packetInterval
```

**Example** (default: 64 bytes, 1.0 s interval):
```
DataRate = 64 × 8 / 1.0 = 512 bps = 0.512 kbps per node
```

With 10 mesh nodes, total offered load:
```
TotalOfferedLoad = 10 × 512 = 5,120 bps = 5.12 kbps
```

This is **2.05% of the 250 kbps channel capacity**, confirming the simulation tests routing-layer performance, not raw channel saturation.

---

## 6. Measurement Methodology — Dual Counting

The simulation measures packet counts using **two independent mechanisms** to cross-validate results:

### Method A: NS-3 Trace Callbacks

Direct event hooks attached to the application layer:

```
PacketSentCallback     → increments g_totalSent     (fires on every Tx)
PacketReceivedCallback → increments g_totalReceived  (fires on every Rx at sink)
```

These callbacks fire at the **application layer**, meaning they count packets from the perspective of the source and sink applications, regardless of what happens in the network layers beneath.

### Method B: NS-3 FlowMonitor

FlowMonitor operates at the **IP layer**. It intercepts all IPv6 packets, tags them with a unique flow identifier, and records:

| Field | Description |
|---|---|
| `txPackets` | Packets sent by source (IP layer) |
| `rxPackets` | Packets received at sink (IP layer) |
| `txBytes` / `rxBytes` | Byte counts |
| `delaySum` | Cumulative sum of per-packet delays (nanoseconds internally) |
| `lostPackets` | Packets that never arrived |

FlowMonitor groups traffic into **flows**, where each flow is a unique `(sourceIP, destIP, sourcePort, destPort, protocol)` tuple. With `nMesh` source nodes all sending to the same Border Router on the same port, there are `nMesh` flows.

### Why Two Methods?

| Aspect | Callback | FlowMonitor |
|---|---|---|
| Layer | Application | IP |
| Captures routing overhead | No | Yes |
| Accurate for delay | No | Yes |
| Used for quality classification | No | **Yes** |
| Useful for sanity check | **Yes** | Yes |

Discrepancy between `PDR_CB` and `PDR_FM` can indicate:
- Retransmissions being counted differently
- AODV control packets interfering
- 6LoWPAN fragmentation/reassembly failures

---

## 7. Simulation Scenarios

The bash script `run_scenarios.sh` runs three independent parameter sweeps. In each scenario, **one variable changes** while all others are held constant (controlled experiment).

### 7.1 Scenario 1 — Node Count Sweep

**Question:** At what node count does the Thread mesh start degrading?

| Fixed Parameter | Value |
|---|---|
| nodeSpacing | 20 m |
| nPackets | 60 |
| simTime | 60 s |
| packetSize | 64 bytes |
| packetInterval | 1.0 s |

**Sweep values for `totalNodes`:** 2, 3, 5, 6, 11, 16, 21, 26, 31

Note: `totalNodes = N` means `N-1` mesh source nodes (1 is always the Border Router).

**Physical interpretation of 20 m spacing with N nodes:**
- Rightmost node (mesh node `N-2`) is at distance `(N-1) × 20` metres from BR
- E.g., with `totalNodes=31`: last node is at `30 × 20 = 600 m` — requires multiple hops
- AODV must discover multi-hop routes; each additional hop adds delay and loss probability

**Expected behaviour:**
- Small N → all nodes within 1–2 hops → EXCELLENT
- Medium N → longer chains → more AODV overhead, possible GOOD
- Large N → deep chains → route failures → DEGRADED/POOR

### 7.2 Scenario 2 — Node Spacing Sweep

**Question:** What is the maximum inter-node distance before the mesh fails?

| Fixed Parameter | Value |
|---|---|
| totalNodes | 6 (1 BR + 5 mesh) |
| nPackets | 60 |
| simTime | 60 s |
| packetSize | 64 bytes |
| packetInterval | 1.0 s |

**Sweep values for `nodeSpacing` (metres):** 5, 10, 15, 20, 30, 40, 50, 60, 80, 100

**Path loss interpretation:**

At spacing `d` metres, nodes in the linear grid are at distances `d, 2d, 3d, 4d, 5d` from the BR. The furthest node (Mesh 5) is at `5d` from the BR. Without intermediate relay, path loss from that node directly to BR:

```
PathLoss (dB) = 20·log₁₀(5d) + 20·log₁₀(2.4×10⁹) - 147.55
```

| Spacing (d) | Furthest distance (5d) | Path Loss (approx.) |
|---|---|---|
| 10 m | 50 m | 74.0 dB |
| 20 m | 100 m | 80.0 dB |
| 30 m | 150 m | 83.5 dB |
| 50 m | 250 m | 88.0 dB |
| 80 m | 400 m | 92.1 dB |
| 100 m | 500 m | 94.0 dB |

When path loss exceeds the link budget of the 802.15.4 radio (~85–90 dB in NS-3's default model), the direct link fails and AODV must route through intermediaries. If spacing is too large for even 1-hop relay, the node becomes **unreachable** → PDR → 0.

### 7.3 Scenario 3 — Packet Load Sweep

**Question:** How much traffic load can the mesh handle before congestion causes packet loss?

| Fixed Parameter | Value |
|---|---|
| totalNodes | 6 (1 BR + 5 mesh) |
| nodeSpacing | 20 m |
| simTime | 60 s |
| packetSize | 64 bytes |
| packetInterval | 1.0 s |

**Sweep values for `nPackets`:** 10, 20, 30, 50, 80, 100, 150, 200

**Offered load calculation per sweep point:**

Since all packets must arrive within `simTime = 60 s`:

```
OfferedLoad_total (kbps) = nMesh × nPackets × packetSize × 8 / (simTime × 1000)
                         = 5 × nPackets × 64 × 8 / (60 × 1000)
                         = 5 × nPackets × 0.08533  kbps
```

| nPackets | Total Offered Load |
|---|---|
| 10 | 4.27 kbps |
| 50 | 21.3 kbps |
| 100 | 42.7 kbps |
| 200 | 85.3 kbps |

As offered load approaches and exceeds the effective mesh capacity (significantly less than 250 kbps due to CSMA-CA overhead, ACKs, routing traffic), MAC buffer overflows increase and PDR drops.

---

## 8. Threshold Detection Algorithm

The bash script automatically detects and reports the **exact transition point** where quality degrades from one tier to the next. This is done using a state-machine approach:

```bash
PREV_QUALITY = ""

for each sweep_value X:
    run simulation → get PDR, Delay
    QLABEL = classify(PDR, Delay)   # EXCELLENT / GOOD / DEGRADED / POOR

    if QLABEL == "EXCELLENT":
        LAST_EXCELLENT = X          # track last X value that was EXCELLENT

    if QLABEL != PREV_QUALITY AND PREV_QUALITY != "":
        # Quality just changed — record the transition point
        if PREV_QUALITY == "EXCELLENT":
            THRESHOLD_EX = "crossed at X (was OK up to PREV_X)"
        elif PREV_QUALITY == "GOOD":
            THRESHOLD_GD = "crossed at X (was OK up to PREV_X)"
        elif PREV_QUALITY == "DEGRADED":
            THRESHOLD_DG = "crossed at X (was OK up to PREV_X)"

    PREV_QUALITY = QLABEL
    PREV_X = X
```

**Important note:** The sweep points are not guaranteed to be evenly spaced — the threshold is bracketed between two tested values (not interpolated). For example, if quality is EXCELLENT at `spacing=30m` and GOOD at `spacing=40m`, the true threshold lies somewhere between 30 m and 40 m. To find the precise boundary, additional simulations between these two values would be needed.

---

## 9. File Structure & Outputs

### CSV Output Columns — `results_nodes.csv` / `results_spacing.csv` / `results_packets.csv`

| Column | Unit | Description |
|---|---|---|
| `TotalNodes` | — | Total nodes including Border Router |
| `MeshNodes` | — | Number of source nodes (= TotalNodes - 1) |
| `PDR_FM_pct` | % | PDR measured by FlowMonitor |
| `PacketLoss_pct` | % | = 100 - PDR_FM_pct |
| `AvgDelay_ms` | ms | Mean end-to-end delay across all flows |
| `NetThroughput_kbps` | kbps | Total data delivered to BR per second |
| `PerNodeTput_kbps` | kbps | NetThroughput / MeshNodes |
| `QualityLabel` | — | EXCELLENT / GOOD / DEGRADED / POOR |

### Master Log — `thread_pdr_results.csv`

Contains all columns above plus:

| Column | Description |
|---|---|
| `Timestamp` | Wall-clock time when the simulation ran |
| `SimTime_s` | Simulation duration in seconds |
| `PacketSize_bytes` | UDP payload size |
| `PacketInterval_s` | Time between packets from each source |
| `NodeSpacing_m` | Grid spacing (0 if random layout) |
| `AreaSize_m` | Random area side length (ignored if grid mode) |
| `NPackets_per_node` | Max packets per source (0 = unlimited) |
| `Sent_CB` / `Recv_CB` / `Lost_CB` | Callback-layer counts |
| `PDR_CB_pct` | Callback PDR |
| `Sent_FM` / `Recv_FM` / `Lost_FM` | FlowMonitor counts |
| `TxBytes` / `RxBytes` | Total bytes transferred |

---

## 10. Installation & Prerequisites

### Required Software

| Software | Version | Purpose |
|---|---|---|
| NS-3 | 3.36 or 3.40 | Core simulation engine |
| GCC / Clang | C++17 compatible | Compiling the `.cc` file |
| Python 3 | 3.8+ | NS-3 build system (`./ns3`) |
| Bash | 4.0+ | Running the scenario script |
| `awk` | GNU awk | CSV column extraction in script |
| `column` | util-linux | Formatting output tables |

### NS-3 Module Dependencies

The following NS-3 modules must be included in the build (enabled by default in standard NS-3 installs):

```
core          network         internet
mobility      lr-wpan         sixlowpan
applications  aodv            flow-monitor
```

### Directory Setup

Edit the first line of `run_scenarios.sh` to point to your NS-3 directory:

```bash
NS3_DIR="$HOME/ns-allinone-3.36/ns-3.36"   # ← CHANGE THIS
```

---

## 11. How to Run

### Step 1 — Copy Simulation File

The script automatically copies the `.cc` file to the NS-3 scratch directory:

```bash
cp thread-pdr-simulation.cc "$NS3_DIR/scratch/"
```

Or do it manually:

```bash
cp thread-pdr-simulation.cc ~/ns-allinone-3.36/ns-3.36/scratch/
```

### Step 2 — Run All Scenarios (Recommended)

```bash
chmod +x run_scenarios.sh
./run_scenarios.sh
```

This runs all three scenarios and takes approximately **20–40 minutes** depending on your hardware (each individual simulation runs ~60 simulated seconds in real time).

### Step 3 — Run a Single Simulation (Optional)

Navigate to your NS-3 directory and use:

```bash
cd ~/ns-allinone-3.36/ns-3.36

# Default run (11 nodes, 60 packets, random placement)
./ns3 run "thread-pdr-simulation"

# Custom run — 6 nodes, 20 m grid spacing, 50 packets per node
./ns3 run "thread-pdr-simulation --totalNodes=6 --nodeSpacing=20 --nPackets=50"

# Custom run — 21 nodes, 100 packets, 30-second simulation
./ns3 run "thread-pdr-simulation --totalNodes=21 --nPackets=100 --simTime=30"

# Custom run — large random area with 16 nodes
./ns3 run "thread-pdr-simulation --totalNodes=16 --areaSize=100 --nPackets=60"
```

### Complete CLI Parameter Reference

| Parameter | Default | Description |
|---|---|---|
| `--totalNodes` | 11 | Total nodes including Border Router (min 2) |
| `--nPackets` | 0 | Packets per mesh node (0 = run until simTime) |
| `--nodeSpacing` | 0.0 | Grid spacing in metres (0 = random placement) |
| `--simTime` | 60.0 | Simulation duration in seconds |
| `--packetSize` | 64 | UDP payload size in bytes |
| `--packetInterval` | 1.0 | Time between packets in seconds |
| `--areaSize` | 50.0 | Side length of random placement area in metres |
| `--outputFile` | `thread_pdr_results.csv` | Path for CSV output |

---

## 12. Understanding the Output

### Console Output Structure

When a simulation runs, the console prints in this order:

```
════════════════════════════════════════════════
  Thread Mesh PDR + Throughput Simulation
════════════════════════════════════════════════
  [Configuration banner — shows all parameters]
════════════════════════════════════════════════

════════════════════════════════════════════════
  Per-Flow Results
════════════════════════════════════════════════
  FlowID  Sent   Recv   Lost  PDR(%)  Delay(ms)  Tput(kbps)
  ──────────────────────────────────────────────────────────
  [One row per mesh node flow]
════════════════════════════════════════════════

  OVERALL RESULTS
  ─────────────
  [Callback stats]
  [FlowMonitor aggregate stats]
  [Throughput stats]

****************************************************
  NETWORK QUALITY ASSESSMENT (Auto-Calculated)
****************************************************
  [Quality tier table]
  [Measured values]
  [Quality Label]
  [Threshold Report]
****************************************************
```

### Interpreting the Threshold Summary

At the end of `run_scenarios.sh`, the summary block shows:

```
Scenario 1: Node Count Threshold (nodeSpacing=20m)
  Mesh is PERFECTLY WORKING (EXCELLENT) up to totalNodes = 6
  Mesh is still GOOD (usable) up to totalNodes = 11
  >> EXCELLENT -> GOOD threshold   : crossed at totalNodes=11 (was OK up to 6)
  >> GOOD -> DEGRADED threshold    : crossed at totalNodes=16 (was OK up to 11)
```

**Reading this:**
- 1–6 nodes: deploy freely, performance is optimal
- 7–11 nodes: still usable but no longer perfect
- 12–16 nodes: performance degrades significantly
- 17+ nodes: network becomes unreliable

---

## 13. Worked Example with Sample Numbers

Suppose a run with `totalNodes=6, nodeSpacing=20, nPackets=60, simTime=60` produces:

```
FlowMonitor: Sent = 300, Received = 291, Lost = 9
delaySum (across 5 flows) = 350 ms total
TotalRxBytes = 18,624 bytes
SimTime = 60 s
nMesh = 5
```

**Step-by-step metric calculations:**

**PDR_FM:**
```
PDR_FM = (291 / 300) × 100 = 97.0 %
```

**PLR:**
```
PLR = 100 - 97.0 = 3.0 %
```

**Average Delay:**
```
AvgDelay = 350 ms / 5 flows = 70 ms
```
(Assuming each of the 5 flows contributes equally to the delay sum)

**Net Throughput:**
```
NetThroughput = (18,624 × 8) / (60 × 1000)
              = 149,192 / 60,000
              = 2.487 kbps
```

**Per-Node Throughput:**
```
PerNodeTput = 2.487 / 5 = 0.497 kbps
```

**Quality Classification:**
```
PDR_FM = 97.0 ≥ 95.0 ✓
AvgDelay = 70 ms < 100 ms ✓
→ Label = EXCELLENT
```

**Expected packets per node:**
```
With nPackets=60 and MaxBytes = 60 × 64 = 3,840 bytes per source
Total expected = 5 × 60 = 300 packets  [matches fmSent=300 ✓]
```

---

## 14. Glossary

| Term | Definition |
|---|---|
| **AODV** | Ad-hoc On-Demand Distance Vector — reactive mesh routing protocol |
| **Border Router (BR)** | The single gateway node in a Thread network; bridges to IPv6 internet |
| **CSMA-CA** | Carrier Sense Multiple Access with Collision Avoidance — MAC access method |
| **FlowMonitor** | NS-3 module that intercepts and records per-flow IP-layer statistics |
| **IEEE 802.15.4** | Wireless standard for low-rate, short-range PHY/MAC (base of Thread, Zigbee) |
| **6LoWPAN** | IPv6 over Low-Power Wireless Personal Area Networks — compression layer |
| **LR-WPAN** | Low-Rate Wireless Personal Area Network — NS-3's 802.15.4 implementation |
| **NS-3** | Network Simulator 3 — open-source discrete-event network simulator |
| **OnOff Application** | NS-3 traffic generator that alternates between transmitting and idle states |
| **PAN** | Personal Area Network — the logical 802.15.4 network domain (identified by PAN ID) |
| **PDR** | Packet Delivery Ratio — fraction of sent packets successfully received |
| **PLR** | Packet Loss Rate = 1 - PDR |
| **RREQ / RREP / RERR** | AODV control messages: Route Request, Route Reply, Route Error |
| **Thread** | IoT mesh networking protocol built on 802.15.4 + 6LoWPAN + IPv6 |
| **Throughput** | Rate of successful data delivery (kbps) |
| **UDP** | User Datagram Protocol — connectionless transport used by sources |

---

