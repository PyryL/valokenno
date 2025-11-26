//
//  TimestampParser.swift
//  valokenno
//
//  Created by Pyry Lahtinen on 12.7.2025.
//

class TimestampParser {
    static func parse(_ timestamps: String, deviceCount: Int) throws -> [[UInt32]] {
        let devices = timestamps.split(separator: ";")

        guard devices.count == deviceCount else {
            throw ParseError.invalid
        }

        var result: [[UInt32]] = []

        for i in 0..<deviceCount {
            let deviceTimestamps = try parseDevice(String(devices[i]), deviceNumber: i+1)
            result.append(deviceTimestamps)
        }

        return result
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
