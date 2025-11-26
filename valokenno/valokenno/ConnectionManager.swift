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

    private let urlSession: URLSession

    init() {
        urlSession = URLSession(configuration: .ephemeral)
        urlSession.configuration.timeoutIntervalForRequest = 2.0 // TODO: this does not work
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

    public func getTimestamps() async -> String? {
        // check that connected at the beginning
        guard let ssid = await getCurrentWiFiSSID(), ssid == valokennoSSID else {
            return nil
        }

        guard let startUrl = URL(string: "\(baseUrl)/timestamps"),
              let resultUrl = URL(string: "\(baseUrl)/timestamps/result") else {

            return nil
        }

        let sessionConfig = URLSessionConfiguration.ephemeral
        sessionConfig.timeoutIntervalForRequest = 1.0
        let session = URLSession(configuration: sessionConfig)

        guard let (_, response) = try? await session.data(from: startUrl) else {
            return nil
        }

        guard let httpResponse = response as? HTTPURLResponse, httpResponse.statusCode == 200 else {
            return nil
        }

        try? await Task.sleep(nanoseconds: 1_500_000_000) // 1.5 seconds

        // if not connected, wait until connected
        guard await waitForNetworkRestore() else {
            return nil
        }

        // with 10 retries, try to get the results from device
        for i in 0..<10 {
            if i > 0 {
                do {
                    try await Task.sleep(nanoseconds: 1_000_000_000) // 1 second
                } catch {
                    return nil
                }
            }

            // if we lose connection during requests, wait until the connection restores
            guard await waitForNetworkRestore() else {
                return nil
            }

            // allow slightly longer timeouts for each request
            let sessionConfig = URLSessionConfiguration.ephemeral
            sessionConfig.timeoutIntervalForRequest = 1.0 + Double(i)
            let session = URLSession(configuration: sessionConfig)

            guard let (data, response) = try? await session.data(from: resultUrl) else {
                continue
            }

            guard let httpResponse = response as? HTTPURLResponse, httpResponse.statusCode == 200 else {
                continue
            }

            guard let string = String(data: data, encoding: .utf8) else {
                continue
            }

            return string
        }

        return nil
    }

    public func clearTimestamps() async -> Bool {
        guard let startUrl = URL(string: "\(baseUrl)/clear"),
              let resultUrl = URL(string: "\(baseUrl)/clear/result") else {

            return false
        }

        guard let (_, response) = try? await urlSession.data(from: startUrl) else {
            return false
        }

        guard let httpResponse = response as? HTTPURLResponse, httpResponse.statusCode == 200 else {
            return false
        }


        for _ in 0..<5 {
            do {
                try await Task.sleep(nanoseconds: 1_000_000_000) // 1 second
            } catch {
                return false
            }

            guard let (data, response) = try? await urlSession.data(from: resultUrl) else {
                continue
            }

            guard let httpResponse = response as? HTTPURLResponse, httpResponse.statusCode == 200 else {
                continue
            }

            guard let string = String(data: data, encoding: .utf8) else {
                continue
            }

            return string == "ok"
        }

        return false
    }

    /// Returns once the phone is connected to the Valokenno Wi-Fi (returning true), of after the timeout of 10 seconds (returning false).
    private func waitForNetworkRestore() async -> Bool {
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
                       let ssid = await self.getCurrentWiFiSSID(),
                       ssid == self.valokennoSSID {

                        timeoutItem.cancel()
                        monitor.cancel()
                        resume(true)
                    }
                }
            }

            monitor.start(queue: queue)
            queue.asyncAfter(deadline: .now() + 10, execute: timeoutItem) // 10 sec timeout
        }
    }

    private func getCurrentWiFiSSID() async -> String? {
        return await withCheckedContinuation { cont in
            NEHotspotNetwork.fetchCurrent { network in
                if let network {
                    cont.resume(returning: network.ssid)
                } else {
                    cont.resume(returning: nil)
                }
            }
        }
    }
}
