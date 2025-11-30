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

    public func getTimestamps() async -> String? {
        guard let startUrl = URL(string: "\(baseUrl)/timestamps"),
              let resultUrl = URL(string: "\(baseUrl)/timestamps/result") else {

            return nil
        }

        guard let (_, response) = try? await urlSession.data(from: startUrl) else {
            return nil
        }

        guard let httpResponse = response as? HTTPURLResponse, httpResponse.statusCode == 200 else {
            return nil
        }


        for _ in 0..<5 {
            do {
                try await Task.sleep(nanoseconds: 1_000_000_000) // 1 second
            } catch {
                return nil
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

    public func activateStarter() async -> Bool {
        guard let url = URL(string: "\(baseUrl)/starter") else {
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

        return string == "ok"
    }
}
