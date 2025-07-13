//
//  TimestampParser.swift
//  valokenno
//
//  Created by Pyry Lahtinen on 12.7.2025.
//

class TimestampParser {
    static func parse(_ timestamps: String) throws -> ([UInt32], [UInt32]) {
        let devices = timestamps.split(separator: ";")

        guard devices.count == 2 else {
            throw ParseError.invalid
        }

        let timestamps1 = try parseDevice(String(devices[0]), deviceNumber: 1)
        let timestamps2 = try parseDevice(String(devices[1]), deviceNumber: 2)

        return (timestamps1, timestamps2)
    }

    static private func parseDevice(_ string: String, deviceNumber: Int) throws -> [UInt32] {
        let parts = string.split(separator: ",")

        guard let firstPart = parts.first, firstPart == "dev\(deviceNumber)" else {
            throw ParseError.invalid
        }

        var timestamps: [UInt32] = []

        for part in parts.dropFirst() {
            guard let timestamp = UInt32(part) else {
                throw ParseError.invalid
            }
            timestamps.append(timestamp)
        }

        return timestamps
    }

    enum ParseError: Error {
        case invalid
    }
}
