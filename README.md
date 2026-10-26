# SmartMesh

A custom routing protocol for wireless mesh networks, developed and evaluated using the ns-3 network simulator.

## Overview

SmartMesh explores routing decisions for wireless mesh networks using network-performance metrics. The project includes a custom routing implementation and simulation scenarios for comparison with Ad hoc On-Demand Distance Vector (AODV) routing.

## Features

- Custom SmartMesh routing protocol implemented in C++.
- Integration with the ns-3 network simulator.
- AODV baseline comparison.
- Light, moderate, and heavy traffic scenarios.
- Evaluation using packet delivery ratio (PDR), delay, throughput, and packet drops.
- Flow Monitor and packet-trace support.
- Python scripts and graphs for result analysis.

## Project Structure

- `src/smartmesh-routing.cc` — custom routing implementation.
- `src/smartmesh-routing.h` — routing protocol declaration.
- `src/smartmesh.cc` — SmartMesh simulation scenario.
- `src/smartmesh-light.cc` — light-traffic scenario.
- `src/smartmesh-moderate.cc` — moderate-traffic scenario.
- `src/smartmesh-heavy.cc` — heavy-traffic scenario.
- `src/aodv-light.cc` — AODV light-traffic baseline.
- `src/aodv-moderate.cc` — AODV moderate-traffic baseline.
- `src/aodv-heavy.cc` — AODV heavy-traffic baseline.
- `results/graphs/` — comparison graphs and plotting script.

## Requirements

- C++ compiler compatible with your ns-3 version.
- ns-3 network simulator.
- Python 3 and Matplotlib for regenerating graphs.

## Running

The source files are designed to run with ns-3. Build and execution commands depend on your ns-3 installation and version. Ensure the custom routing implementation is available to the ns-3 build before running the scenarios.

## Evaluation

The included scenarios support comparison between SmartMesh and AODV under different traffic loads. Relevant metrics include:

- Packet Delivery Ratio (PDR)
- End-to-end delay
- Throughput
- Packet drops

Comparison graphs are provided in `results/graphs/`.

## Project Status

Academic prototype under development. Performance results should be interpreted in the context of the implemented simulation, topology, traffic parameters, and experimental configuration.
