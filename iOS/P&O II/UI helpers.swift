//
//  UI helpers.swift
//  P&O II
//
//  Created by Bram Roelandts on 13/03/2020.
//  Copyright © 2020 Bram Roelandts. All rights reserved.
//

import UIKit
import LocalAuthentication


// MARK: - Error management

func displayError(title: String, message: String) {
    
    // Get a reference to the top controller
    if let topViewController = UIApplication.shared.keyWindow?.rootViewController?.presentingViewController {
        // Create alertView
        let alertController = UIAlertController(title: title, message: message, preferredStyle: .alert)
        alertController.addAction(UIAlertAction(title: "Dismiss", style: .default))
        topViewController.present(alertController, animated: true, completion: nil)
    }
}


// MARK: - Authentication management

func authenticateBiometrically(completion: @escaping (_ success: Bool) -> Void) {
    
    let context = LAContext()
    var error: NSError?
    
    if context.canEvaluatePolicy(.deviceOwnerAuthenticationWithBiometrics, error: &error) {
        context.evaluatePolicy(.deviceOwnerAuthenticationWithBiometrics, localizedReason: "Alleen geauthoriseerd gebruik van het AAA is toegestaan.") { (success, authenticationError) in
            DispatchQueue.main.async {
                if success {
                    completion(success)
                } else {
                    // Fallback selected because biometrics failed
                    authenticatePasscode { (passcodeSuccess) in
                        completion(passcodeSuccess)
                    }
                }
            }
        }
    } else {
        
        // Use passcode, because no biometrics found
        authenticatePasscode { (passcodeSuccess) in
            completion(passcodeSuccess)
        }
    }
}

fileprivate func authenticatePasscode(completion: @escaping (_ success: Bool) -> Void) {
    
    let context = LAContext()
    context.evaluatePolicy(.deviceOwnerAuthentication, localizedReason: "Alleen geauthoriseerd gebruik van het AAA.") { (success, error) in
        DispatchQueue.main.async {
            completion(success)
        }
    }
}

