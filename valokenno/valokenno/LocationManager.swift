//
//  LocationManager.swift
//  valokenno
//
//  Created by Pyry Lahtinen on 15.11.2025.
//

import Foundation
import CoreLocation

class LocationManager: NSObject, CLLocationManagerDelegate {
    override init() {
        super.init()
        manager.delegate = self
    }

    private let manager = CLLocationManager()

    public func requestAuthorization() {
        // location authorization is required to obtain the SSID of the current Wi-Fi connection
        manager.requestWhenInUseAuthorization()
    }

    func locationManagerDidChangeAuthorization(_ manager: CLLocationManager) {
        print("Location authorization \(manager.authorizationStatus) \(manager.accuracyAuthorization)")
    }
}
