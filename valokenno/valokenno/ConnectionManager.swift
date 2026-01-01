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

    public func getTimestamps() async throws -> ([String:[UInt32]], String?) {
        guard let startUrl = URL(string: "\(baseUrl)/timestamps"),
              let resultUrl = URL(string: "\(baseUrl)/timestamps/result") else {

            throw ConnectionError.couldNotStartProcess
        }

        guard let (_, response) = try? await urlSession.data(from: startUrl) else {
            throw ConnectionError.couldNotStartProcess
        }

        guard let httpResponse = response as? HTTPURLResponse, httpResponse.statusCode == 200 else {
            throw ConnectionError.couldNotStartProcess
        }


        for _ in 0..<5 {
            try? await Task.sleep(nanoseconds: 1_000_000_000) // 1 second

            guard let (data, response) = try? await urlSession.data(from: resultUrl) else {
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

    enum ConnectionError: Error {
        case invalidResponseFormat, couldNotStartProcess, couldNotReceiveResponseInTime

        var description: String {
            switch self {
            case .invalidResponseFormat:
                "Master device responded in invalid format"
            case .couldNotStartProcess:
                "Could not connect the master device to initialize the action"
            case .couldNotReceiveResponseInTime:
                "Master device did not respond in time"
            }
        }
    }
}
