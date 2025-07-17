//
//  ConnectionManager.swift
//  valokenno
//
//  Created by Pyry Lahtinen on 17.7.2025.
//

import Foundation

class ConnectionManager {
    private let baseUrl = "http://192.168.4.1"

    private let urlSession: URLSession

    init() {
        urlSession = URLSession(configuration: .ephemeral)
        urlSession.configuration.timeoutIntervalForRequest = 2.0
    }

    public func checkConnection() async -> Bool {
        guard let url = URL(string: "\(baseUrl)/status") else {
            return false
        }

        guard let (data, response) = try? await urlSession.data(from: url) else {
            return false
        }

        guard let httpResponse = response as? HTTPURLResponse, httpResponse.statusCode == 200 else {
            return false
        }

        guard let string = String(data: data, encoding: .utf8) else {
            return false
        }

        return string == "Valokenno toiminnassa"
    }
}
