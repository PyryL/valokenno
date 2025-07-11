//
//  BluetoothManager.swift
//  valokenno
//
//  Created by Pyry Lahtinen on 10.7.2025.
//

import CoreBluetooth

class BluetoothManager: NSObject, CBCentralManagerDelegate, CBPeripheralDelegate {
    private let manager = CBCentralManager()

    override init() {
        super.init()
        manager.delegate = self
    }

    private let serviceId = CBUUID(string: "6459c4ca-d023-43ca-a8d4-c43710315b7f")
    private let characteristicId = CBUUID(string: "5e076e58-df1d-4630-b418-74079207a520")

    public var stateCallback: (() -> Void)?

    private var readCallback: ((String) -> Void)?

    private(set) var state: State = .notStarted

    private var peripheral: CBPeripheral?
    private var service: CBService?
    private var characteristic: CBCharacteristic?

    func start() {
        guard state == .notStarted || state == .waitingPower else {
            return
        }

        if manager.state == .poweredOn {
            print("Scanning started")
            state = .scanning
            manager.scanForPeripherals(withServices: [serviceId])
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

    func readValue(_ callback: @escaping (String) -> Void) {
        guard state == .ready, readCallback == nil, let peripheral, peripheral.state == .connected, let characteristic else {
            return
        }
        readCallback = callback

        peripheral.readValue(for: characteristic)
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

        peripheral.delegate = self
        peripheral.discoverServices([serviceId])
    }

    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: (any Error)?) {
        self.peripheral = nil
        service = nil
        characteristic = nil
        state = .scanning
        stateCallback?()
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: (any Error)?) {
        if let service = peripheral.services?.first {
            self.service = service
            peripheral.discoverCharacteristics([characteristicId], for: service)
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: (any Error)?) {
        if let characteristic = service.characteristics?.first {
            self.characteristic = characteristic
            state = .ready
            stateCallback?()
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: (any Error)?) {
        if let data = characteristic.value, let string = String(data: data, encoding: .utf8) {
            readCallback?(string)
            readCallback = nil
        }
    }

    enum State {
        case notStarted, waitingPower, scanning, connecting, connected, ready
    }
}
