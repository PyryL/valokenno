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

    func readValue() {
        manager.readValue { value in
            print("Read value \"\(value)\"")
        }
    }

    var body: some View {
        VStack {
            Image(systemName: "globe")
                .imageScale(.large)
                .foregroundStyle(.tint)

            Text(managerStateDescription)

            Button(action: readValue) {
                Label("Read value", systemImage: "globe")
            }
        }
        .padding()
        .onAppear {
            manager.stateCallback = {
                switch manager.state {
                case .notStarted, .waitingPower:
                    managerStateDescription = "Starting..."
                case .scanning:
                    managerStateDescription = "Scanning..."
                case .connecting:
                    managerStateDescription = "Connecting..."
                case .connected:
                    managerStateDescription = "Initing..."
                case .ready:
                    managerStateDescription = "Ready!"
                }
            }
            manager.start()
        }
    }
}

#Preview {
    ContentView()
}
