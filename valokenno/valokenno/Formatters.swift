//
//  Formatters.swift
//  valokenno
//
//  Created by Pyry Lahtinen on 17.7.2025.
//

import Foundation

class Formatters {
    public static func formatTimestamp(_ timestamp: UInt32, digits: Int = 3) -> String {
        let formatter = NumberFormatter()
        formatter.minimumFractionDigits = digits
        formatter.maximumFractionDigits = digits
        formatter.usesGroupingSeparator = false
        let seconds = Double(timestamp) / 1000
        return formatter.string(from: seconds as NSNumber) ?? "\(seconds)"
    }
}
