//
//  StarterView.swift
//  valokenno
//
//  Created by Pyry Lahtinen on 29.11.2025.
//

import SwiftUI

struct StarterView: View {
    @Binding var isVisible: Bool
    var connectionManager: ConnectionManager

    @State private var state: StarterState = .notActivated
    @State private var isFailureAlertVisible: Bool = false

    private func activate() {
        guard state == .notActivated else {
            return
        }
        state = .activating

        Task {
            if await connectionManager.activateStarter() {
                state = .activated
            } else {
                state = .notActivated
                isFailureAlertVisible = true
            }
        }
    }

    private var buttonLabel: some View {
        Group {
            if state == .notActivated {
                Text("Start")
            } else if state == .activating {
                HStack {
                    ProgressView()
                        .progressViewStyle(.circular)
                    Text("Wait...")
                }
            } else if state == .activated {
                Text("Activated")
            }
        }
    }

    private var buttonColor: Color {
        switch state {
        case .notActivated:
            .green
        case .activating:
            .gray
        case .activated:
            .red
        }
    }

    var body: some View {
        NavigationStack {
            Button {
                activate()
            } label: {
                buttonLabel
                    .padding()
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
                    .background(buttonColor)
                    .clipShape(RoundedRectangle(cornerRadius: 16.0))
            }
            .font(.largeTitle)
            .buttonStyle(.plain)
            .padding()

            .navigationTitle("Starter")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .topBarTrailing) {
                    Button {
                        isVisible = false
                    } label: {
                        Text("Close")
                    }
                }
            }
            .alert("Starter failed", isPresented: $isFailureAlertVisible) {
                Button("OK") { }
            }
        }
    }

    private enum StarterState {
        case notActivated, activating, activated
    }
}

#Preview {
    StarterView(isVisible: .constant(true), connectionManager: ConnectionManager())
}
