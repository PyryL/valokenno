//
//  ContentView.swift
//  valokenno
//
//  Created by Pyry Lahtinen on 10.7.2025.
//

import SwiftUI

struct ContentView: View {
    let manager = BluetoothManager()
    @State var managerStateDescription: String = "Starting..."
    @State var managerStateColor: Color = .red
    @State var timestamps: ([UInt32], [UInt32])? = nil

    func readValue() {
        timestamps = nil
        manager.readValue { value in
            print("Read value \"\(value)\"")
            do {
                let (t1, t2) = try TimestampParser.parse(value)
                timestamps = (t1, t2)
            } catch {
                print("parse failed")
            }
        }
    }

    private func formatTimestamp(_ timestamp: UInt32) -> String {
        let formatter = NumberFormatter()
        formatter.minimumFractionDigits = 4
        formatter.maximumFractionDigits = 4
        formatter.usesGroupingSeparator = false
        let seconds = Double(timestamp) / 1000
        return formatter.string(from: seconds as NSNumber) ?? "\(seconds)"
    }

    var body: some View {
        NavigationStack {
            List {
                Section {
                    if let timestamps {
                        ForEach(timestamps.0, id: \.self) { timestamp in
                            Text("\(formatTimestamp(timestamp))")
                        }
                    }
                } header: {
                    Text("Device 1")
                }

                Section {
                    if let timestamps {
                        ForEach(timestamps.1, id: \.self) { timestamp in
                            Text("\(formatTimestamp(timestamp))")
                        }
                    }
                } header: {
                    Text("Device 2")
                }
            }
            .navigationTitle("Valokenno")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .topBarLeading) {
                    HStack {
                        Circle()
                            .frame(width: 32, height: 32)
                            .foregroundStyle(managerStateColor)
                        Text(managerStateDescription)
                    }
                }
                ToolbarItem(placement: .topBarTrailing) {
                    Button(action: readValue) {
                        Label("Read value", systemImage: "arrow.down.circle")
                            .labelStyle(.titleAndIcon)
                            .padding()
                    }
                }
            }
        }
        .onAppear {
            manager.stateCallback = {
                switch manager.state {
                case .notStarted, .waitingPower:
                    managerStateDescription = "Starting..."
                    managerStateColor = .red
                case .scanning:
                    managerStateDescription = "Scanning..."
                    managerStateColor = .red
                case .connecting:
                    managerStateDescription = "Connecting..."
                    managerStateColor = .orange
                case .connected:
                    managerStateDescription = "Initing..."
                    managerStateColor = .orange
                case .ready:
                    managerStateDescription = "Ready!"
                    managerStateColor = .green
                }
            }
            manager.start()
        }
    }
}

#Preview {
    ContentView()
}
