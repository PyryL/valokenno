//
//  ContentView.swift
//  valokenno
//
//  Created by Pyry Lahtinen on 10.7.2025.
//

import SwiftUI

struct ContentView: View {
    let manager = ConnectionManager()
    @State var connectionStatus: ConnectionStatus = .none
    @State var timestamps: ([UInt32], [UInt32])? = nil
    @State var selectedTimestampDevice1: UInt32? = nil
    @State var selectedTimestampDevice2: UInt32? = nil

    @State var isLoadingTimestamps: Bool = false
    @State var isClearingTimestamps: Bool = false

    private var connectionStatusLabel: some View {
        Group {
            if connectionStatus == .none {
                Image(systemName: "questionmark")
                    .foregroundStyle(.primary)
            } else if connectionStatus == .connecting {
                ProgressView()
                    .progressViewStyle(.circular)
            } else if connectionStatus == .connected {
                Image(systemName: "checkmark.circle.fill")
                    .foregroundStyle(.green)
            } else if connectionStatus == .notConnected {
                Image(systemName: "xmark")
                    .foregroundStyle(.red)
            }
        }
    }

    private func checkConnection() {
        connectionStatus = .connecting
        Task {
            let isConnected = await manager.checkConnection()
            connectionStatus = isConnected ? .connected : .notConnected
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
                    connectionStatusLabel
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

    enum ConnectionStatus {
        case none, connecting, connected, notConnected
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
                Button {
                    selectedTimestamp = timestamp
                } label: {
                    ZStack(alignment: .leading) {
                        Color.white
                            .opacity(0.000001)

                        HStack {
                            Image(systemName: "checkmark")
                                .opacity(timestamp == selectedTimestamp ? 1 : 0)

                            Text("\(Formatters.formatTimestamp(timestamp))")
                                .monospaced()
                        }
                    }
                }
                .buttonStyle(.plain)
            }
        }
    }
}

#Preview {
    ContentView()
}
