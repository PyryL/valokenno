//
//  BluetoothManager.swift
//  valokenno
//
//  Created by Pyry Lahtinen on 10.7.2025.
//

import CoreBluetooth

class BluetoothManager: NSObject, CBCentralManagerDelegate {
    private let manager = CBCentralManager()

    override init() {
        super.init()
        manager.delegate = self
    }

    private let service = CBUUID(string: "6459c4ca-d023-43ca-a8d4-c43710315b7f")

    public var stateCallback: (() -> Void)?

    private(set) var state: State = .notStarted

    private var peripheral: CBPeripheral?

    func start() {
        guard state == .notStarted || state == .waitingPower else {
            return
        }

        if manager.state == .poweredOn {
            print("Scanning started")
            state = .scanning
            manager.scanForPeripherals(withServices: [service])
        } else {
            state = .waitingPower
        }

        stateCallback?()
    }

    private func connect(peripheral: CBPeripheral) {
        guard state == .scanning else {
            return
        }
        state = .connecting
        self.peripheral = peripheral

        stateCallback?()

        manager.stopScan()
        manager.connect(peripheral)
    }


    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        print("Bluetooth state \(central.state)")
        if central.state == .poweredOn, state == .waitingPower {
            start()
        }
    }

    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String : Any], rssi RSSI: NSNumber) {
        if state == .scanning {
            connect(peripheral: peripheral)
        }
    }

    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        state = .connected
        stateCallback?()
    }

    enum State {
        case notStarted, waitingPower, scanning, connecting, connected
    }
}
