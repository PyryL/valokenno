//
//  ConnectionManager.swift
//  valokenno
//
//  Created by Pyry Lahtinen on 17.7.2025.
//

import Foundation
import Network
import NetworkExtension

class ConnectionManager {
    private let baseUrl = "http://192.168.4.1"
    private let valokennoSSID = "Valokenno"

    public func checkConnection() async -> Bool {
        guard await isCurrentNetworkValokennoWiFi() else {
            return false
        }

        guard let url = URL(string: "\(baseUrl)/status") else {
            return false
        }

        let sessionConfig = URLSessionConfiguration.ephemeral
        sessionConfig.timeoutIntervalForRequest = 2.0
        let session = URLSession(configuration: sessionConfig)

        defer {
            session.invalidateAndCancel()
        }

        guard let (data, response) = try? await session.data(from: url) else {
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

    public func getTimestamps() async throws -> ([String:[UInt32]], String?) {
        // check that connected at the beginning
        guard await isCurrentNetworkValokennoWiFi() else {
            throw ConnectionError.initiallyNotConnectedToWifi
        }

        guard let startUrl = URL(string: "\(baseUrl)/timestamps"),
              let resultUrl = URL(string: "\(baseUrl)/timestamps/result") else {

            throw ConnectionError.couldNotStartProcess
        }

        // this do block is used to defer the session before entering the retry loop below
        do {
            let sessionConfig = URLSessionConfiguration.ephemeral
            sessionConfig.timeoutIntervalForRequest = 1.0
            let session = URLSession(configuration: sessionConfig)

            defer {
                session.invalidateAndCancel()
            }

            guard let (_, response) = try? await session.data(from: startUrl) else {
                throw ConnectionError.couldNotStartProcess
            }

            guard let httpResponse = response as? HTTPURLResponse, httpResponse.statusCode == 200 else {
                throw ConnectionError.couldNotStartProcess
            }
        }

        try? await Task.sleep(nanoseconds: 1_500_000_000) // 1.5 seconds

        // if not connected, wait until connected
        guard await waitForNetworkRestore() else {
            throw ConnectionError.couldNotReconnectToWifi
        }

        // with 5 retries, try to get the results from device
        for i in 0..<5 {
            if i > 0 {
                try? await Task.sleep(nanoseconds: 1_000_000_000) // 1 second
            }

            // if we lose connection during requests, wait until the connection restores
            guard await waitForNetworkRestore(timeout: 5.0) else {
                throw ConnectionError.couldNotReconnectToWifi
            }

            // allow slightly longer timeouts for each request
            let sessionConfig = URLSessionConfiguration.ephemeral
            sessionConfig.timeoutIntervalForRequest = 1.0 + Double(i)
            let session = URLSession(configuration: sessionConfig)

            defer {
                session.invalidateAndCancel()
            }

            guard let (data, response) = try? await session.data(from: resultUrl) else {
                continue
            }

            guard let httpResponse = response as? HTTPURLResponse, httpResponse.statusCode == 200 else {
                continue
            }

            guard let object = try? JSONSerialization.jsonObject(with: data) as? [String:Any],
                  let timestamps = object["timestamps"] as? [String:[UInt32]],
                  let errorMessage = object["error"] as? String else {

                throw ConnectionError.invalidResponseFormat
            }

            let optionalErrorMessage = errorMessage.isEmpty ? nil : errorMessage

            return (timestamps, optionalErrorMessage)
        }

        throw ConnectionError.couldNotReceiveResponseInTime
    }

    public func clearTimestamps() async throws {
        // check that connected at the beginning
        guard await isCurrentNetworkValokennoWiFi() else {
            throw ConnectionError.initiallyNotConnectedToWifi
        }

        guard let startUrl = URL(string: "\(baseUrl)/clear"),
              let resultUrl = URL(string: "\(baseUrl)/clear/result") else {

            throw ConnectionError.couldNotStartProcess
        }

        // use do block to defer session before entering the retry loop below
        do {
            let sessionConfig = URLSessionConfiguration.ephemeral
            sessionConfig.timeoutIntervalForRequest = 2.0
            let session = URLSession(configuration: sessionConfig)

            defer {
                session.invalidateAndCancel()
            }

            guard let (_, response) = try? await session.data(from: startUrl) else {
                throw ConnectionError.couldNotStartProcess
            }

            guard let httpResponse = response as? HTTPURLResponse, httpResponse.statusCode == 200 else {
                throw ConnectionError.couldNotStartProcess
            }
        }

        try? await Task.sleep(nanoseconds: 1_500_000_000) // 1.5 seconds

        // if not connected, wait until connected
        guard await waitForNetworkRestore() else {
            throw ConnectionError.couldNotReconnectToWifi
        }

        for i in 0..<5 {
            if i > 0 {
                try? await Task.sleep(nanoseconds: 1_000_000_000) // 1 second
            }

            // if we lose connection during requests, wait until the connection restores
            guard await waitForNetworkRestore(timeout: 5.0) else {
                throw ConnectionError.couldNotReconnectToWifi
            }

            let sessionConfig = URLSessionConfiguration.ephemeral
            sessionConfig.timeoutIntervalForRequest = 1.0 + Double(i)
            let session = URLSession(configuration: sessionConfig)

            defer {
                session.invalidateAndCancel()
            }

            guard let (data, response) = try? await session.data(from: resultUrl) else {
                continue
            }

            guard let httpResponse = response as? HTTPURLResponse, httpResponse.statusCode == 200 else {
                continue
            }

            guard let object = try? JSONSerialization.jsonObject(with: data) as? [String:Any],
                  let success = object["success"] as? Bool,
                  let errorMessage = object["error"] as? String else {

                throw ConnectionError.invalidResponseFormat
            }

            if !errorMessage.isEmpty {
                throw ConnectionError.masterReportedError(errorMessage)
            }

            guard success else {
                // if success is false, error message should be provided
                throw ConnectionError.invalidResponseFormat
            }

            return
        }

        throw ConnectionError.couldNotReceiveResponseInTime
    }

    public func activateStarter() async throws {
        // check that connected to wifi
        guard await isCurrentNetworkValokennoWiFi() else {
            throw ConnectionError.initiallyNotConnectedToWifi
        }

        guard let url = URL(string: "\(baseUrl)/starter") else {
            throw ConnectionError.couldNotStartProcess
        }

        let sessionConfig = URLSessionConfiguration.ephemeral
        sessionConfig.timeoutIntervalForRequest = 2.0
        let session = URLSession(configuration: sessionConfig)

        defer {
            session.invalidateAndCancel()
        }

        guard let (data, response) = try? await session.data(from: url) else {
            throw ConnectionError.couldNotStartProcess
        }

        guard let httpResponse = response as? HTTPURLResponse, httpResponse.statusCode == 200 else {
            throw ConnectionError.invalidResponseFormat
        }

        guard let string = String(data: data, encoding: .utf8), string == "ok" else {
            throw ConnectionError.invalidResponseFormat
        }
    }

    enum ConnectionError: Error {
        case invalidResponseFormat, couldNotStartProcess, couldNotReceiveResponseInTime, initiallyNotConnectedToWifi, couldNotReconnectToWifi, masterReportedError(String)

        var description: String {
            switch self {
            case .invalidResponseFormat:
                "Master device responded in invalid format"
            case .couldNotStartProcess:
                "Could not connect the master device to initialize the action"
            case .couldNotReceiveResponseInTime:
                "Master device did not respond in time"
            case .initiallyNotConnectedToWifi:
                "Could not initialize the action due to not being connected to WiFi"
            case .couldNotReconnectToWifi:
                "Could not reconnect to WiFi to finish the action"
            case .masterReportedError(let message):
                message
            }
        }
    }

    /// Returns once the phone is connected to the Valokenno Wi-Fi (returning true), or after the timeout (returning false).
    private func waitForNetworkRestore(timeout: Double = 10.0) async -> Bool {
        return await withCheckedContinuation { cont in
            let monitor = NWPathMonitor()
            let queue = DispatchQueue(label: "info.pyry.apps.valokenno.ConnectionManager.waitForNetworkRestore")

            let lock = NSLock()
            var hasResumed = false

            let resume = { result in
                lock.lock()

                if !hasResumed {
                    hasResumed = true
                    cont.resume(returning: result)
                }

                lock.unlock()
            }

            let timeoutItem = DispatchWorkItem {
                monitor.cancel()
                resume(false)
            }

            monitor.pathUpdateHandler = { path in
                Task {
                    if path.status == .satisfied,
                       path.usesInterfaceType(.wifi),
                       await self.isCurrentNetworkValokennoWiFi() {

                        timeoutItem.cancel()
                        monitor.cancel()
                        resume(true)
                    }
                }
            }

            monitor.start(queue: queue)
            queue.asyncAfter(deadline: .now() + timeout, execute: timeoutItem)
        }
    }

    /// Returns `true` if the currently connected network is the Valokenno WiFi. Otherwise returns `false`.
    private func isCurrentNetworkValokennoWiFi() async -> Bool {
        let maxAttempts = 3

        for i in 0..<maxAttempts {
            let ssid = await withCheckedContinuation { cont in
                NEHotspotNetwork.fetchCurrent { network in
                    cont.resume(returning: network?.ssid)
                }
            }

            if ssid == self.valokennoSSID {
                return true
            }

            if i < maxAttempts - 1 {
                try? await Task.sleep(nanoseconds: 100_000_000) // 100ms
            }
        }

        return false
    }
}
