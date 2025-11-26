//
//  ContentView.swift
//  valokenno
//
//  Created by Pyry Lahtinen on 10.7.2025.
//

import SwiftUI

struct ContentView: View {
    let manager = ConnectionManager()
    @State var timestampManager = TimestampManager(deviceCount: 3)
    @State var connectionStatus: ConnectionStatus = .none

    @State var isClearConfirmAlertVisible: Bool = false

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

        timestampManager.clear()
        isLoadingTimestamps = true

        Task {
            if let responseString = await manager.getTimestamps() {
                try timestampManager.setTimestamps(response: responseString)
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
                timestampManager.clear()
            }
            isClearingTimestamps = false
        }
    }

    var body: some View {
        NavigationStack {
            VStack {
                List {
                    ForEach(0..<timestampManager.deviceCount, id: \.self) { deviceIndex in
                        Section {
                            TimestampList(timestampManager: timestampManager, deviceIndex: deviceIndex)
                        } header: {
                            Text("Device \(deviceIndex+1)")
                        }
                    }
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
                    Button {
                        isClearConfirmAlertVisible = true
                    } label: {
                        ZStack {
                            ProgressView()
                                .progressViewStyle(.circular)
                                .opacity(isClearingTimestamps ? 1 : 0)

                            Image(systemName: "trash")
                                .opacity(isClearingTimestamps ? 0 : 1)
                        }
                    }
                }
                ToolbarItem(placement: .topBarTrailing) {
                    Button(action: getTimestamps) {
                        ZStack {
                            ProgressView()
                                .progressViewStyle(.circular)
                                .opacity(isLoadingTimestamps ? 1 : 0)

                            Image(systemName: "arrow.down.circle")
                                .opacity(isLoadingTimestamps ? 0 : 1)
                        }
                    }
                }
            }
            .alert("Clear timestamps?", isPresented: $isClearConfirmAlertVisible) {
                Button(role: .cancel, action: {}) {
                    Text("Cancel")
                }
                Button(role: .destructive) {
                    clearTimestamps()
                } label: {
                    Text("Clear")
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
    var timestampManager: TimestampManager
    var deviceIndex: Int

    var body: some View {
        if let timestamps = timestampManager.timestamps(for: deviceIndex) {
            if timestamps.isEmpty {
                Text("No timestamps")
            } else {
                ForEach(timestamps, id: \.self) { timestamp in
                    Button {
                        timestampManager.selectTimestamp(timestamp, device: deviceIndex)
                    } label: {
                        ZStack(alignment: .leading) {
                            Color.white
                                .opacity(0.000001)

                            HStack {
                                Image(systemName: "checkmark")
                                    .opacity(timestamp == timestampManager.selectedTimestamp(for: deviceIndex) ? 1 : 0)

                                Text("\(Formatters.formatTimestamp(timestamp))")
                                    .monospacedDigit()
                            }
                        }
                    }
                    .buttonStyle(.plain)
                }

                if deviceIndex > 0 {
                    HStack {
                        Spacer()

                        Image(systemName: "sum")

                        if let t0 = timestampManager.selectedTimestamp(for: 0), let t1 = timestampManager.selectedTimestamp(for: deviceIndex), t1 >= t0 {
                            Text("\(Formatters.formatTimestamp(t1 - t0, digits: 2))")
                                .monospacedDigit()
                        } else {
                            Text("-.--")
                                .monospaced()
                        }

                        Spacer()

                        Group {
                            Image(systemName: "chevron.forward.dotted.chevron.forward")

                            if let t1 = timestampManager.selectedTimestamp(for: deviceIndex-1), let t2 = timestampManager.selectedTimestamp(for: deviceIndex), t2 >= t1 {
                                Text("\(Formatters.formatTimestamp(t2 - t1, digits: 2))")
                                    .monospacedDigit()
                            } else {
                                Text("-.--")
                                    .monospaced()
                            }
                        }
                        .opacity(deviceIndex > 1 ? 1.0 : 0.0)

                        Spacer()
                    }
                    .bold()
                }
            }
        }
    }
}

@Observable
class TimestampManager {
    init(deviceCount: Int) {
        self.deviceCount = deviceCount
    }

    private(set) var deviceCount: Int

    /// The outer array is empty if no data is available, or contains `deviceCount` many subarrays of timestamps.
    private var timestamps: [[UInt32]] = []

    /// The array is empty if no timestamp data is available.
    /// Whenever `timestamps` is not empty, this array has `deviceCount` values.
    /// `nil` value means no selection.
    private var selectedTimestamps: [UInt32?] = []

    public func setTimestamps(response: String) throws {
        timestamps = try TimestampParser.parse(response, deviceCount: deviceCount)

        selectedTimestamps = Array(repeating: nil, count: deviceCount)

        for i in 0..<deviceCount {
            if timestamps[i].count == 1 {
                selectedTimestamps[i] = timestamps[i].first!
            }
        }
    }

    public func selectTimestamp(_ timestamp: UInt32, device: Int) {
        guard selectedTimestamps.count == deviceCount, device >= 0, device < deviceCount else {
            return
        }
        selectedTimestamps[device] = timestamp
    }

    public func timestamps(for device: Int) -> [UInt32]? {
        guard timestamps.count == deviceCount, device >= 0, device < deviceCount else {
            return nil
        }
        return timestamps[device]
    }

    public func selectedTimestamp(for device: Int) -> UInt32? {
        guard selectedTimestamps.count == deviceCount, device >= 0, device < deviceCount else {
            return nil
        }
        return selectedTimestamps[device]
    }

    public func clear() {
        timestamps = []
        selectedTimestamps = []
    }
}

#Preview {
    let timestampManager = TimestampManager(deviceCount: 3)
    try? timestampManager.setTimestamps(response: "dev1,10000,15000;dev2,12000,17000;dev3,16000,20000")

    return ContentView(timestampManager: timestampManager)
}
