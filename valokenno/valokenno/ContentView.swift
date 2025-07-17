//
//  ContentView.swift
//  valokenno
//
//  Created by Pyry Lahtinen on 10.7.2025.
//

import SwiftUI

struct ContentView: View {
    let manager = ConnectionManager()
    @State var connectionLabel: String = "N/A"
    @State var timestamps: ([UInt32], [UInt32])? = nil

    private func checkConnection() {
        connectionLabel = "Checking..."
        Task {
            let isConnected = await manager.checkConnection()
            connectionLabel = isConnected ? "Connected" : "Not connected"
        }
    }

    private func getTimestamps() {
        timestamps = nil
        Task {
            if let responseString = await manager.getTimestamps() {
                timestamps = try? TimestampParser.parse(responseString)
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
                    Text(connectionLabel)
                        .onTapGesture {
                            checkConnection()
                        }
                }
                ToolbarItem(placement: .topBarTrailing) {
                    Button(action: getTimestamps) {
                        Label("Read value", systemImage: "arrow.down.circle")
                            .labelStyle(.titleAndIcon)
                            .padding()
                    }
                }
            }
        }
        .onAppear {
            checkConnection()
        }
    }
}

#Preview {
    ContentView()
}
