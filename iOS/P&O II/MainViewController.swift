//
//  ViewController.swift
//  P&O II
//
//  Created by Bram Roelandts on 13/03/2020.
//  Copyright © 2020 Bram Roelandts. All rights reserved.
//

import UIKit
import CoreBluetooth



class MainViewController: UIViewController, UITextFieldDelegate, BluetoothSerialDelegate {

    
    // MARK: - Outlets
    
    @IBOutlet var orderButton: UIButton!
    @IBOutlet var resourceSegment: UISegmentedControl!
    @IBOutlet var weigthField: UITextField!
    @IBOutlet var weightIndicatorLabel: UILabel!
    @IBOutlet var weightLabel: UILabel!
    
    var orderProgressView: UIAlertController?
    var serial: BluetoothSerial!
    
    
    // MARK: - ViewDidLoad
    
    override func viewDidLoad() {
        super.viewDidLoad()

        // Setup the navigation bar
        self.title = "AAA - Team 306"
        self.navigationController?.navigationBar.prefersLargeTitles = true
        if #available(iOS 13.0, *) {
            self.view.backgroundColor = .tertiarySystemGroupedBackground
        } else {
            self.view.backgroundColor = .groupTableViewBackground
        }
        
        // Style the order button
        orderButton.layer.cornerRadius = 8
        
        // Setup textfield handling
        weigthField.delegate = self
        
        // Hide monitoring labels at start
        weightLabel.isHidden = true
        weightIndicatorLabel.isHidden = true
        
        // Initialize the BluetoothSerial
        serial = BluetoothSerial(delegate: self)
        
        // Start scanning after initialization
        DispatchQueue.main.asyncAfter(deadline: .now() + 2) {
            self.serial.startScan()
        }
    }
    
    
    // MARK: - Handling
    
    @IBAction func placeOrder(_ sender: Any) {
        
        // Gather all values
        let orderQuantity = Int(weigthField.text ?? "0") ?? 0
        let resource = ResourceType(rawValue: resourceSegment.selectedSegmentIndex) ?? .mais
        
        // Verify data
        if orderQuantity < 1 || orderQuantity > 150 {
            displayError(title: "Ongeldig gewicht", message: "Gelieve een gewicht tussen 0 en 150 gram in te geven.")
            return
        }
        
        // Launch authentication
        authenticateBiometrically { (authSuccess) in
            if authSuccess {
                
                // Clear the field
                self.weigthField.text = ""
                
                // Start the order
                self.launchOrder(resource, orderQuantity)
            
            } else {
                displayError(title: "Authenticatie mislukt", message: "Alleen geauthoriseerd gebruik van de AAA is toegestaan. Gelieve opnieuw te authenticeren.")
            }
        }
    }
    
    func launchOrder(_ resource: ResourceType, _ quantity: Int) {
        
        let instructionByte = UInt8(10)
        let byteResource = UInt8(resource.rawValue)
        let byteQuantity = UInt8(quantity)
        
        // Send the raw bytes
        serial.sendBytesToDevice([instructionByte, byteResource, byteQuantity])
    }
    
    
    // MARK: - Bluetooth communication
    
    func serialDidReceiveString(_ message: String) {
        
        print(message)
        
        // Show labels, if not already shown
        weightIndicatorLabel.isHidden = false
        weightLabel.isHidden = false
        
        // Show current weight
        weightLabel.text = message + " g"
    }
    
    func serialDidDisconnect(_ peripheral: CBPeripheral, error: NSError?) {
        self.serial.connectToPeripheral(peripheral)
    }
    
    func serialDidChangeState() {
        print("Changed state")
        if serial.centralManager.state != .poweredOn {
            print("Bluetooth was turned off")
        }
    }
    
    
    // MARK: - Bluetooth connection
    
    func serialDidDiscoverPeripheral(_ peripheral: CBPeripheral, RSSI: NSNumber?) {
        print(peripheral)
        if peripheral.name == "=Team 306" {
            
            print("Trying to connect")
            // Stop scanning
            serial.stopScan()
            
            self.serial.connectToPeripheral(peripheral)
        }
    }
    
    func serialDidFailToConnect(_ peripheral: CBPeripheral, error: NSError?) {
        print("Connection failed")
    }

    func serialIsReady(_ peripheral: CBPeripheral) {
        print("Serial is ready now")
        serial.sendMessageToDevice("Howdy")
    }
    
    
    
    // MARK: - TextFieldDelegate
    
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        self.view.endEditing(true)
    }
    
    func textFieldShouldReturn(_ textField: UITextField) -> Bool {
        weigthField.resignFirstResponder()
        return true
    }
    
    @objc func dismissKeyboard() {
        self.view.endEditing(true)
    }
}

