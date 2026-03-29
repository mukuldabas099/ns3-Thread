# #!/bin/bash
# # ============================================================
# # Thread Mesh PDR - Run multiple scenarios and collect results
# # ============================================================
# # Usage: bash run_pdr_scenarios.sh
# # Requires: NS-3 installed at ~/ns-3.xx/
# # ============================================================

# NS3_DIR="$HOME/ns-allinone-3.40/ns-3.40"   # ← Change to your NS-3 path
# SCRIPT_NAME="thread-pdr-simulation"
# SCRATCH_DIR="$NS3_DIR/scratch"

# echo "================================================"
# echo "  Thread Mesh PDR - Scenario Runner"
# echo "================================================"

# # Copy simulation file to scratch
# cp thread-pdr-simulation.cc "$SCRATCH_DIR/"

# cd "$NS3_DIR" || exit 1

# # ── Scenario 1: Vary Number of Nodes ──────────────────────
# echo ""
# echo "📡 Scenario 1: Varying Node Count (PDR vs Nodes)"
# echo "-----------------------------------------------------"
# echo "Nodes,PDR,PacketLoss,AvgDelay" > results_nodes.csv

# for N in 5 10 15 20 25 30; do
#     echo -n "  Running with $N nodes... "
#     ./ns3 run "$SCRIPT_NAME --nNodes=$N --simTime=60 --outputFile=/tmp/pdr_n$N.csv" \
#         2>/dev/null
#     PDR=$(grep "PDR_percent" /tmp/pdr_n$N.csv | cut -d',' -f2)
#     LOSS=$(grep "PacketLossRate" /tmp/pdr_n$N.csv | cut -d',' -f2)
#     DELAY=$(grep "AvgDelay_ms" /tmp/pdr_n$N.csv | cut -d',' -f2)
#     echo "$N,$PDR,$LOSS,$DELAY" >> results_nodes.csv
#     echo "PDR=$PDR%"
# done

# # ── Scenario 2: Vary Area Size ─────────────────────────────
# echo ""
# echo "📐 Scenario 2: Varying Area Size (PDR vs Distance)"
# echo "-----------------------------------------------------"
# echo "AreaSize_m,PDR,PacketLoss,AvgDelay" > results_area.csv

# for AREA in 20 30 40 50 60 80 100; do
#     echo -n "  Running with area ${AREA}x${AREA}m... "
#     ./ns3 run "$SCRIPT_NAME --nNodes=10 --areaSize=$AREA --simTime=60 --outputFile=/tmp/pdr_a$AREA.csv" \
#         2>/dev/null
#     PDR=$(grep "PDR_percent" /tmp/pdr_a$AREA.csv | cut -d',' -f2)
#     LOSS=$(grep "PacketLossRate" /tmp/pdr_a$AREA.csv | cut -d',' -f2)
#     DELAY=$(grep "AvgDelay_ms" /tmp/pdr_a$AREA.csv | cut -d',' -f2)
#     echo "$AREA,$PDR,$LOSS,$DELAY" >> results_area.csv
#     echo "PDR=$PDR%"
# done

# # ── Scenario 3: Vary TX Power ──────────────────────────────
# echo ""
# echo "⚡ Scenario 3: Varying TX Power (PDR vs Power)"
# echo "-----------------------------------------------------"
# echo "TxPower_dBm,PDR,PacketLoss,AvgDelay" > results_power.csv

# for PWR in -10 -5 0 5 10; do
#     echo -n "  Running with TX power ${PWR}dBm... "
#     ./ns3 run "$SCRIPT_NAME --nNodes=10 --txPower=$PWR --simTime=60 --outputFile=/tmp/pdr_p$PWR.csv" \
#         2>/dev/null
#     PDR=$(grep "PDR_percent" /tmp/pdr_p$PWR.csv | cut -d',' -f2)
#     LOSS=$(grep "PacketLossRate" /tmp/pdr_p$PWR.csv | cut -d',' -f2)
#     DELAY=$(grep "AvgDelay_ms" /tmp/pdr_p$PWR.csv | cut -d',' -f2)
#     echo "$PWR,$PDR,$LOSS,$DELAY" >> results_power.csv
#     echo "PDR=$PDR%"
# done


# # ── Print Summary ──────────────────────────────────────────
# echo ""
# echo "================================================"
# echo "  Results Summary"
# echo "================================================"
# echo ""
# echo "📊 PDR vs Node Count:"
# cat results_nodes.csv
# echo ""
# echo "📊 PDR vs Area Size:"
# cat results_area.csv
# echo ""
# echo "📊 PDR vs TX Power:"
# cat results_power.csv

# echo ""
# echo "✅ All scenarios complete! CSV files saved."









# #!/bin/bash
# # ============================================================
# # Thread Mesh PDR + Throughput - Scenario Runner
# # ============================================================
# # Usage:
# #   bash run_pdr_scenarios.sh
# #
# # All scenarios write ONE CSV per scenario type:
# #   results_nodes.csv   - PDR/Throughput vs Node count
# #   results_spacing.csv - PDR/Throughput vs Node spacing
# #   results_packets.csv - PDR/Throughput vs Packets/node
# #
# # The simulation also appends every run to:
# #   thread_pdr_results.csv   (full-detail master log)
# # ============================================================

# NS3_DIR="$HOME/ns-allinone-3.40/ns-3.40"   # ← Change to your NS-3 path
# SCRIPT_NAME="thread-pdr-simulation"
# SCRATCH_DIR="$NS3_DIR/scratch"

# # ── Shared defaults used in all scenarios ──────────────────
# SIM_TIME=60
# PACKET_SIZE=64
# PKT_INTERVAL=1.0
# PDR_THRESHOLD=80      # WARN printed when PDR drops below this

# echo "========================================================"
# echo "  Thread Mesh PDR + Throughput - Scenario Runner"
# echo "========================================================"

# # Copy latest simulation file into NS-3 scratch
# cp thread-pdr-simulation.cc "$SCRATCH_DIR/"

# cd "$NS3_DIR" || exit 1

# # ── Helper: extract a value from the per-run CSV ────────────
# # Usage: get_col <file> <column_name>
# # The CSV is horizontal (header on row 1), so we use awk.
# get_col() {
#     local FILE="$1"
#     local COL="$2"
#     awk -F',' -v col="$COL" '
#         NR==1 { for(i=1;i<=NF;i++) if($i==col) idx=i }
#         NR==2 { print $idx }
#     ' "$FILE"
# }

# # ============================================================
# # Scenario 1: Vary Total Nodes (fixed spacing = 20 m)
# # Command equivalent:
# #   ./ns3 run "thread-pdr-simulation --totalNodes=N --nodeSpacing=20 ..."
# # ============================================================
# echo ""
# echo "📡 Scenario 1: PDR + Throughput vs Total Nodes"
# echo "   (nodeSpacing=20m, nPackets=60, pdrThreshold=$PDR_THRESHOLD)"
# echo "--------------------------------------------------------"
# echo "TotalNodes,MeshNodes,PDR_FM_pct,PacketLoss_pct,AvgDelay_ms,NetThroughput_kbps,PerNodeTput_kbps,QualityPass" \
#     > results_nodes.csv

# for N in 2 5 6 11 16 21 26 31; do
#     TMPCSV="/tmp/pdr_nodes_n${N}.csv"
#     echo -n "  totalNodes=$N ... "
#     ./ns3 run "$SCRIPT_NAME \
#         --totalNodes=$N \
#         --nodeSpacing=20 \
#         --nPackets=60 \
#         --simTime=$SIM_TIME \
#         --packetSize=$PACKET_SIZE \
#         --packetInterval=$PKT_INTERVAL \
#         --pdrThreshold=$PDR_THRESHOLD \
#         --outputFile=$TMPCSV" 2>/dev/null

#     PDR=$(get_col   "$TMPCSV" "PDR_FM_pct")
#     LOSS=$(get_col  "$TMPCSV" "PacketLoss_pct")
#     DELAY=$(get_col "$TMPCSV" "AvgDelay_ms")
#     TPUT=$(get_col  "$TMPCSV" "NetThroughput_kbps")
#     PNTPUT=$(get_col "$TMPCSV" "PerNodeTput_kbps")
#     PASS=$(get_col  "$TMPCSV" "QualityPass")
#     MESH=$((N - 1))

#     echo "$N,$MESH,$PDR,$LOSS,$DELAY,$TPUT,$PNTPUT,$PASS" >> results_nodes.csv
#     echo "PDR=${PDR}%  Tput=${TPUT}kbps  [$PASS]"
# done

# # ============================================================
# # Scenario 2: Vary Node Spacing / Distance
# # Command equivalent:
# #   ./ns3 run "thread-pdr-simulation --totalNodes=6 --nodeSpacing=D ..."
# # ============================================================
# echo ""
# echo "📐 Scenario 2: PDR + Throughput vs Node Spacing (Distance)"
# echo "   (totalNodes=6 → 5 mesh, nPackets=60)"
# echo "--------------------------------------------------------"
# echo "NodeSpacing_m,PDR_FM_pct,PacketLoss_pct,AvgDelay_ms,NetThroughput_kbps,PerNodeTput_kbps,QualityPass" \
#     > results_spacing.csv

# for SPACING in 5 10 15 20 30 40 50 60 80 100; do
#     TMPCSV="/tmp/pdr_spacing_s${SPACING}.csv"
#     echo -n "  nodeSpacing=${SPACING}m ... "
#     ./ns3 run "$SCRIPT_NAME \
#         --totalNodes=6 \
#         --nodeSpacing=$SPACING \
#         --nPackets=60 \
#         --simTime=$SIM_TIME \
#         --packetSize=$PACKET_SIZE \
#         --packetInterval=$PKT_INTERVAL \
#         --pdrThreshold=$PDR_THRESHOLD \
#         --outputFile=$TMPCSV" 2>/dev/null

#     PDR=$(get_col   "$TMPCSV" "PDR_FM_pct")
#     LOSS=$(get_col  "$TMPCSV" "PacketLoss_pct")
#     DELAY=$(get_col "$TMPCSV" "AvgDelay_ms")
#     TPUT=$(get_col  "$TMPCSV" "NetThroughput_kbps")
#     PNTPUT=$(get_col "$TMPCSV" "PerNodeTput_kbps")
#     PASS=$(get_col  "$TMPCSV" "QualityPass")

#     echo "$SPACING,$PDR,$LOSS,$DELAY,$TPUT,$PNTPUT,$PASS" >> results_spacing.csv
#     echo "PDR=${PDR}%  Tput=${TPUT}kbps  [$PASS]"
# done

# # ============================================================
# # Scenario 3: Vary Packets per Node (fixed 6 nodes, 20m spacing)
# # Command equivalent:
# #   ./ns3 run "thread-pdr-simulation --totalNodes=6 --nPackets=P ..."
# # ============================================================
# echo ""
# echo "📦 Scenario 3: PDR + Throughput vs Packets per Node"
# echo "   (totalNodes=6, nodeSpacing=20m)"
# echo "--------------------------------------------------------"
# echo "NPackets_per_node,PDR_FM_pct,PacketLoss_pct,AvgDelay_ms,NetThroughput_kbps,PerNodeTput_kbps,QualityPass" \
#     > results_packets.csv

# for PKTS in 10 20 30 50 80 100 150 200; do
#     TMPCSV="/tmp/pdr_pkts_p${PKTS}.csv"
#     echo -n "  nPackets=$PKTS ... "
#     ./ns3 run "$SCRIPT_NAME \
#         --totalNodes=6 \
#         --nodeSpacing=20 \
#         --nPackets=$PKTS \
#         --simTime=$SIM_TIME \
#         --packetSize=$PACKET_SIZE \
#         --packetInterval=$PKT_INTERVAL \
#         --pdrThreshold=$PDR_THRESHOLD \
#         --outputFile=$TMPCSV" 2>/dev/null

#     PDR=$(get_col   "$TMPCSV" "PDR_FM_pct")
#     LOSS=$(get_col  "$TMPCSV" "PacketLoss_pct")
#     DELAY=$(get_col "$TMPCSV" "AvgDelay_ms")
#     TPUT=$(get_col  "$TMPCSV" "NetThroughput_kbps")
#     PNTPUT=$(get_col "$TMPCSV" "PerNodeTput_kbps")
#     PASS=$(get_col  "$TMPCSV" "QualityPass")

#     echo "$PKTS,$PDR,$LOSS,$DELAY,$TPUT,$PNTPUT,$PASS" >> results_packets.csv
#     echo "PDR=${PDR}%  Tput=${TPUT}kbps  [$PASS]"
# done

# # ── Print Summary ──────────────────────────────────────────
# echo ""
# echo "========================================================"
# echo "  Results Summary"
# echo "========================================================"

# echo ""
# echo "📊 Scenario 1 — PDR + Throughput vs Node Count:"
# column -t -s',' results_nodes.csv

# echo ""
# echo "📊 Scenario 2 — PDR + Throughput vs Node Spacing:"
# column -t -s',' results_spacing.csv

# echo ""
# echo "📊 Scenario 3 — PDR + Throughput vs Packets per Node:"
# column -t -s',' results_packets.csv

# echo ""
# echo "========================================================"
# echo "  CSV files saved:"
# echo "    results_nodes.csv"
# echo "    results_spacing.csv"
# echo "    results_packets.csv"
# echo "    thread_pdr_results.csv  (master log — all runs)"
# echo "========================================================"
# echo "✅ All scenarios complete!"

















#!/bin/bash
# ============================================================
# Thread Mesh PDR + Throughput - Scenario Runner
# ============================================================
# Runs 3 sweeps and finds the threshold breakpoint automatically:
#
#   Scenario 1: PDR + Throughput vs Total Nodes
#   Scenario 2: PDR + Throughput vs Node Spacing (distance)
#   Scenario 3: PDR + Throughput vs Packets per Node
#
# For each scenario the script finds the exact point where
# quality drops from EXCELLENT -> GOOD -> DEGRADED -> POOR
# and prints a clear threshold summary at the end.
#
# Output files:
#   results_nodes.csv     - sweep 1 data
#   results_spacing.csv   - sweep 2 data
#   results_packets.csv   - sweep 3 data
#   thread_pdr_results.csv - full master log (all runs combined)
# ============================================================

NS3_DIR="$HOME/ns-allinone-3.36/ns-3.36"   # <- Change to your NS-3 path
SCRIPT_NAME="thread-pdr-simulation"
SCRATCH_DIR="$NS3_DIR/scratch"

SIM_TIME=60
PACKET_SIZE=64
PKT_INTERVAL=1.0

echo "================================================================"
echo "  Thread Mesh PDR + Throughput - Scenario Runner"
echo "  Threshold is calculated automatically from simulation results."
echo "================================================================"

cp thread-pdr-simulation.cc "$SCRATCH_DIR/"
cd "$NS3_DIR" || exit 1

# ── Helper: read a column value from a single-row CSV ──────────
# Usage: get_col <csvfile> <ColumnName>
get_col() {
    local FILE="$1"
    local COL="$2"
    awk -F',' -v col="$COL" '
        NR==1 { for(i=1;i<=NF;i++) if($i==col) idx=i }
        NR==2 { if(idx) print $idx }
    ' "$FILE"
}

# ── Helper: classify quality from PDR and delay ────────────────
# Mirrors the C++ ClassifyNetwork() logic exactly.
# Usage: classify_quality <pdr_float> <delay_float>
classify_quality() {
    local PDR="$1"
    local DELAY="$2"
    # Use awk for float comparison
    awk -v pdr="$PDR" -v delay="$DELAY" 'BEGIN {
        if (pdr >= 95 && delay < 100)       print "EXCELLENT"
        else if (pdr >= 85 && delay < 500)  print "GOOD"
        else if (pdr >= 60)                 print "DEGRADED"
        else                                print "POOR"
    }'
}

# ============================================================
# Scenario 1: Vary Total Nodes  (fixed spacing = 20 m)
# Sweeps from 2 nodes (1 mesh) up to 31 nodes (30 mesh)
# ============================================================
echo ""
echo "================================================================"
echo " Scenario 1: PDR + Throughput  vs  Total Nodes"
echo "   (nodeSpacing=20m, nPackets=60, simTime=${SIM_TIME}s)"
echo "================================================================"

HEADER="TotalNodes,MeshNodes,PDR_FM_pct,PacketLoss_pct,AvgDelay_ms,NetThroughput_kbps,PerNodeTput_kbps,QualityLabel"
echo "$HEADER" > results_nodes.csv

# Track threshold crossings
PREV_QUALITY_N=""
THRESHOLD_N_EX=""   # node count where EXCELLENT ends
THRESHOLD_N_GD=""   # node count where GOOD ends
THRESHOLD_N_DG=""   # node count where DEGRADED ends
LAST_EXCELLENT_N=""
LAST_GOOD_N=""

for N in 2 3 5 6 11 16 21 26 31; do
    TMPCSV="/tmp/pdr_scen1_n${N}.csv"
    printf "  totalNodes=%-4s ... " "$N"

    ./ns3 run "$SCRIPT_NAME \
        --totalNodes=$N \
        --nodeSpacing=20 \
        --nPackets=60 \
        --simTime=$SIM_TIME \
        --packetSize=$PACKET_SIZE \
        --packetInterval=$PKT_INTERVAL \
        --outputFile=$TMPCSV" 2>/dev/null

    PDR=$(get_col   "$TMPCSV" "PDR_FM_pct")
    LOSS=$(get_col  "$TMPCSV" "PacketLoss_pct")
    DELAY=$(get_col "$TMPCSV" "AvgDelay_ms")
    TPUT=$(get_col  "$TMPCSV" "NetThroughput_kbps")
    PNTPUT=$(get_col "$TMPCSV" "PerNodeTput_kbps")
    QLABEL=$(classify_quality "$PDR" "$DELAY")
    MESH=$((N - 1))

    echo "$N,$MESH,$PDR,$LOSS,$DELAY,$TPUT,$PNTPUT,$QLABEL" >> results_nodes.csv
    printf "PDR=%6s%%  Delay=%8sms  Tput=%8skbps  [%s]\n" \
           "$PDR" "$DELAY" "$TPUT" "$QLABEL"

    # Detect quality crossings
    if [ "$QLABEL" = "EXCELLENT" ]; then LAST_EXCELLENT_N="$N"; fi
    if [ "$QLABEL" = "GOOD" ];      then LAST_GOOD_N="$N"; fi

    if [ -n "$PREV_QUALITY_N" ] && [ "$QLABEL" != "$PREV_QUALITY_N" ]; then
        case "$PREV_QUALITY_N" in
            EXCELLENT) THRESHOLD_N_EX="crossed at totalNodes=${N} (was OK up to ${PREV_N})" ;;
            GOOD)      THRESHOLD_N_GD="crossed at totalNodes=${N} (was OK up to ${PREV_N})" ;;
            DEGRADED)  THRESHOLD_N_DG="crossed at totalNodes=${N} (was OK up to ${PREV_N})" ;;
        esac
    fi
    PREV_QUALITY_N="$QLABEL"
    PREV_N="$N"
done

# ============================================================
# Scenario 2: Vary Node Spacing / Distance
# (fixed 6 total nodes = 1 BR + 5 mesh)
# ============================================================
echo ""
echo "================================================================"
echo " Scenario 2: PDR + Throughput  vs  Node Spacing (Distance)"
echo "   (totalNodes=6 -> 5 mesh, nPackets=60, simTime=${SIM_TIME}s)"
echo "================================================================"

HEADER="NodeSpacing_m,PDR_FM_pct,PacketLoss_pct,AvgDelay_ms,NetThroughput_kbps,PerNodeTput_kbps,QualityLabel"
echo "$HEADER" > results_spacing.csv

PREV_QUALITY_S=""
THRESHOLD_S_EX=""
THRESHOLD_S_GD=""
THRESHOLD_S_DG=""
LAST_EXCELLENT_S=""
LAST_GOOD_S=""

for SPACING in 5 10 15 20 30 40 50 60 80 100; do
    TMPCSV="/tmp/pdr_scen2_s${SPACING}.csv"
    printf "  nodeSpacing=%-5sm ... " "$SPACING"

    ./ns3 run "$SCRIPT_NAME \
        --totalNodes=6 \
        --nodeSpacing=$SPACING \
        --nPackets=60 \
        --simTime=$SIM_TIME \
        --packetSize=$PACKET_SIZE \
        --packetInterval=$PKT_INTERVAL \
        --outputFile=$TMPCSV" 2>/dev/null

    PDR=$(get_col    "$TMPCSV" "PDR_FM_pct")
    LOSS=$(get_col   "$TMPCSV" "PacketLoss_pct")
    DELAY=$(get_col  "$TMPCSV" "AvgDelay_ms")
    TPUT=$(get_col   "$TMPCSV" "NetThroughput_kbps")
    PNTPUT=$(get_col "$TMPCSV" "PerNodeTput_kbps")
    QLABEL=$(classify_quality "$PDR" "$DELAY")

    echo "$SPACING,$PDR,$LOSS,$DELAY,$TPUT,$PNTPUT,$QLABEL" >> results_spacing.csv
    printf "PDR=%6s%%  Delay=%8sms  Tput=%8skbps  [%s]\n" \
           "$PDR" "$DELAY" "$TPUT" "$QLABEL"

    if [ "$QLABEL" = "EXCELLENT" ]; then LAST_EXCELLENT_S="$SPACING"; fi
    if [ "$QLABEL" = "GOOD" ];      then LAST_GOOD_S="$SPACING"; fi

    if [ -n "$PREV_QUALITY_S" ] && [ "$QLABEL" != "$PREV_QUALITY_S" ]; then
        case "$PREV_QUALITY_S" in
            EXCELLENT) THRESHOLD_S_EX="crossed at spacing=${SPACING}m (was OK up to ${PREV_S}m)" ;;
            GOOD)      THRESHOLD_S_GD="crossed at spacing=${SPACING}m (was OK up to ${PREV_S}m)" ;;
            DEGRADED)  THRESHOLD_S_DG="crossed at spacing=${SPACING}m (was OK up to ${PREV_S}m)" ;;
        esac
    fi
    PREV_QUALITY_S="$QLABEL"
    PREV_S="$SPACING"
done

# ============================================================
# Scenario 3: Vary Packets per Node
# (fixed 6 total nodes, 20m spacing)
# ============================================================
echo ""
echo "================================================================"
echo " Scenario 3: PDR + Throughput  vs  Packets per Node"
echo "   (totalNodes=6, nodeSpacing=20m, simTime=${SIM_TIME}s)"
echo "================================================================"

HEADER="NPackets_per_node,PDR_FM_pct,PacketLoss_pct,AvgDelay_ms,NetThroughput_kbps,PerNodeTput_kbps,QualityLabel"
echo "$HEADER" > results_packets.csv

PREV_QUALITY_P=""
THRESHOLD_P_EX=""
THRESHOLD_P_GD=""
THRESHOLD_P_DG=""
LAST_EXCELLENT_P=""
LAST_GOOD_P=""

for PKTS in 10 20 30 50 80 100 150 200; do
    TMPCSV="/tmp/pdr_scen3_p${PKTS}.csv"
    printf "  nPackets=%-5s ... " "$PKTS"

    ./ns3 run "$SCRIPT_NAME \
        --totalNodes=6 \
        --nodeSpacing=20 \
        --nPackets=$PKTS \
        --simTime=$SIM_TIME \
        --packetSize=$PACKET_SIZE \
        --packetInterval=$PKT_INTERVAL \
        --outputFile=$TMPCSV" 2>/dev/null

    PDR=$(get_col    "$TMPCSV" "PDR_FM_pct")
    LOSS=$(get_col   "$TMPCSV" "PacketLoss_pct")
    DELAY=$(get_col  "$TMPCSV" "AvgDelay_ms")
    TPUT=$(get_col   "$TMPCSV" "NetThroughput_kbps")
    PNTPUT=$(get_col "$TMPCSV" "PerNodeTput_kbps")
    QLABEL=$(classify_quality "$PDR" "$DELAY")

    echo "$PKTS,$PDR,$LOSS,$DELAY,$TPUT,$PNTPUT,$QLABEL" >> results_packets.csv
    printf "PDR=%6s%%  Delay=%8sms  Tput=%8skbps  [%s]\n" \
           "$PDR" "$DELAY" "$TPUT" "$QLABEL"

    if [ "$QLABEL" = "EXCELLENT" ]; then LAST_EXCELLENT_P="$PKTS"; fi
    if [ "$QLABEL" = "GOOD" ];      then LAST_GOOD_P="$PKTS"; fi

    if [ -n "$PREV_QUALITY_P" ] && [ "$QLABEL" != "$PREV_QUALITY_P" ]; then
        case "$PREV_QUALITY_P" in
            EXCELLENT) THRESHOLD_P_EX="crossed at nPackets=${PKTS} (was OK up to ${PREV_P})" ;;
            GOOD)      THRESHOLD_P_GD="crossed at nPackets=${PKTS} (was OK up to ${PREV_P})" ;;
            DEGRADED)  THRESHOLD_P_DG="crossed at nPackets=${PKTS} (was OK up to ${PREV_P})" ;;
        esac
    fi
    PREV_QUALITY_P="$QLABEL"
    PREV_P="$PKTS"
done

# ============================================================
# Print full data tables
# ============================================================
echo ""
echo "================================================================"
echo "  Full Data Tables"
echo "================================================================"
echo ""
echo "Scenario 1 — PDR vs Node Count:"
column -t -s',' results_nodes.csv
echo ""
echo "Scenario 2 — PDR vs Node Spacing:"
column -t -s',' results_spacing.csv
echo ""
echo "Scenario 3 — PDR vs Packets per Node:"
column -t -s',' results_packets.csv

# ============================================================
# AUTO-CALCULATED THRESHOLD SUMMARY
# This is the key output — tells the user exactly where
# the mesh stops working perfectly.
# ============================================================
echo ""
echo "================================================================"
echo "  AUTO-CALCULATED THRESHOLD SUMMARY"
echo "  (Based on IEEE 802.15.4 / Thread Mesh Quality Tiers)"
echo "  EXCELLENT: PDR>=95% AND delay<100ms"
echo "  GOOD:      PDR>=85% AND delay<500ms"
echo "  DEGRADED:  PDR>=60%"
echo "  POOR:      PDR<60%"
echo "================================================================"
echo ""

# ── Scenario 1 Threshold ──────────────────────────────────────
echo " Scenario 1: Node Count Threshold (nodeSpacing=20m)"
echo " ----------------------------------------------------"
if [ -n "$LAST_EXCELLENT_N" ]; then
    echo "  Mesh is PERFECTLY WORKING (EXCELLENT) up to totalNodes = $LAST_EXCELLENT_N"
else
    echo "  Mesh did NOT reach EXCELLENT quality in any tested configuration."
fi
if [ -n "$LAST_GOOD_N" ] && [ "$LAST_GOOD_N" != "$LAST_EXCELLENT_N" ]; then
    echo "  Mesh is still GOOD (usable) up to totalNodes = $LAST_GOOD_N"
fi
if [ -n "$THRESHOLD_N_EX" ]; then
    echo "  >> EXCELLENT -> GOOD threshold   : $THRESHOLD_N_EX"
fi
if [ -n "$THRESHOLD_N_GD" ]; then
    echo "  >> GOOD -> DEGRADED threshold    : $THRESHOLD_N_GD"
fi
if [ -n "$THRESHOLD_N_DG" ]; then
    echo "  >> DEGRADED -> POOR threshold    : $THRESHOLD_N_DG"
fi
echo ""

# ── Scenario 2 Threshold ──────────────────────────────────────
echo " Scenario 2: Node Spacing Threshold (totalNodes=6)"
echo " ---------------------------------------------------"
if [ -n "$LAST_EXCELLENT_S" ]; then
    echo "  Mesh is PERFECTLY WORKING (EXCELLENT) up to nodeSpacing = ${LAST_EXCELLENT_S}m"
else
    echo "  Mesh did NOT reach EXCELLENT quality in any tested spacing."
fi
if [ -n "$LAST_GOOD_S" ] && [ "$LAST_GOOD_S" != "$LAST_EXCELLENT_S" ]; then
    echo "  Mesh is still GOOD (usable) up to nodeSpacing = ${LAST_GOOD_S}m"
fi
if [ -n "$THRESHOLD_S_EX" ]; then
    echo "  >> EXCELLENT -> GOOD threshold   : $THRESHOLD_S_EX"
fi
if [ -n "$THRESHOLD_S_GD" ]; then
    echo "  >> GOOD -> DEGRADED threshold    : $THRESHOLD_S_GD"
fi
if [ -n "$THRESHOLD_S_DG" ]; then
    echo "  >> DEGRADED -> POOR threshold    : $THRESHOLD_S_DG"
fi
echo ""

# ── Scenario 3 Threshold ──────────────────────────────────────
echo " Scenario 3: Packets/Node Threshold (totalNodes=6, spacing=20m)"
echo " ----------------------------------------------------------------"
if [ -n "$LAST_EXCELLENT_P" ]; then
    echo "  Mesh is PERFECTLY WORKING (EXCELLENT) up to nPackets = $LAST_EXCELLENT_P per node"
else
    echo "  Mesh did NOT reach EXCELLENT quality in any tested packet count."
fi
if [ -n "$LAST_GOOD_P" ] && [ "$LAST_GOOD_P" != "$LAST_EXCELLENT_P" ]; then
    echo "  Mesh is still GOOD (usable) up to nPackets = $LAST_GOOD_P per node"
fi
if [ -n "$THRESHOLD_P_EX" ]; then
    echo "  >> EXCELLENT -> GOOD threshold   : $THRESHOLD_P_EX"
fi
if [ -n "$THRESHOLD_P_GD" ]; then
    echo "  >> GOOD -> DEGRADED threshold    : $THRESHOLD_P_GD"
fi
if [ -n "$THRESHOLD_P_DG" ]; then
    echo "  >> DEGRADED -> POOR threshold    : $THRESHOLD_P_DG"
fi
echo ""

echo "================================================================"
echo "  CSV files saved:"
echo "    results_nodes.csv       - Scenario 1 data"
echo "    results_spacing.csv     - Scenario 2 data"
echo "    results_packets.csv     - Scenario 3 data"
echo "    thread_pdr_results.csv  - Master log (all runs)"
echo "================================================================"
echo "  All scenarios complete."