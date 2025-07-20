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
    @State var selectedTimestampDevice1: UInt32? = nil
    @State var selectedTimestampDevice2: UInt32? = nil

    @State var isLoadingTimestamps: Bool = false
    @State var isClearingTimestamps: Bool = false

    private func checkConnection() {
        connectionLabel = "Checking..."
        Task {
            let isConnected = await manager.checkConnection()
            connectionLabel = isConnected ? "Connected" : "Not connected"
        }
    }

    private func getTimestamps() {
        guard !isLoadingTimestamps else {
            return
        }

        timestamps = nil
        selectedTimestampDevice1 = nil
        selectedTimestampDevice2 = nil
        isLoadingTimestamps = true

        Task {
            if let responseString = await manager.getTimestamps() {
                timestamps = try? TimestampParser.parse(responseString)
            }
            isLoadingTimestamps = false
        }
    }

    private func clearTimestamps() {
        guard !isClearingTimestamps else {
            return
        }

        isClearingTimestamps = true

        Task {
            if await manager.clearTimestamps() {
                timestamps = nil
                selectedTimestampDevice1 = nil
                selectedTimestampDevice2 = nil
            }
            isClearingTimestamps = false
        }
    }

    var body: some View {
        NavigationStack {
            VStack {
                List {
                    Section {
                        if let timestamps {
                            TimestampList(timestamps: timestamps.0, selectedTimestamp: $selectedTimestampDevice1)
                        }
                    } header: {
                        Text("Device 1")
                    }

                    Section {
                        if let timestamps {
                            TimestampList(timestamps: timestamps.1, selectedTimestamp: $selectedTimestampDevice2)
                        }
                    } header: {
                        Text("Device 2")
                    }
                }

                if let selectedTimestampDevice1, let selectedTimestampDevice2, selectedTimestampDevice2 > selectedTimestampDevice1 {
                    Text("\(Formatters.formatTimestamp(selectedTimestampDevice2 - selectedTimestampDevice1, digits: 2))")
                        .monospaced()
                        .bold()
                } else {
                    Text("-,--")
                        .monospaced()
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
                    Button(action: clearTimestamps) {
                        if isClearingTimestamps {
                            ProgressView()
                                .progressViewStyle(.circular)
                        } else {
                            Image(systemName: "trash")
                        }
                    }
                }
                ToolbarItem(placement: .topBarTrailing) {
                    Button(action: getTimestamps) {
                        if isLoadingTimestamps {
                            ProgressView()
                                .progressViewStyle(.circular)
                        } else {
                            Image(systemName: "arrow.down.circle")
                        }
                    }
                }
            }
        }
        .onAppear {
            checkConnection()
        }
    }
}

struct TimestampList: View {
    var timestamps: [UInt32]
    @Binding var selectedTimestamp: UInt32?

    var body: some View {
        if timestamps.isEmpty {
            Text("No timestamps")
        } else {
            ForEach(timestamps, id: \.self) { timestamp in
                HStack {
                    Image(systemName: "checkmark")
                        .opacity(timestamp == selectedTimestamp ? 1 : 0)
                    Text("\(Formatters.formatTimestamp(timestamp))")
                        .monospaced()
                }
                .onTapGesture {
                    selectedTimestamp = timestamp
                }
            }
        }
    }
}

#Preview {
    ContentView()
}
