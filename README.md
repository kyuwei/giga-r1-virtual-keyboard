# GIGA R1 Virtual Keyboard — Clavier Virtuel WiFi pour Radxa Q6A

[![License: CC BY-NC 4.0](https://img.shields.io/badge/License-CC%20BY--NC%204.0-lightgrey.svg)](https://creativecommons.org/licenses/by-nc/4.0/)
[![Arduino GIGA R1 WiFi](https://img.shields.io/badge/Board-Arduino%20GIGA%20R1%20WiFi-teal)](https://store.arduino.cc/products/giga-r1-wifi)

Clavier virtuel accessible depuis un **navigateur web**, qui envoie les frappes en **USB HID** vers un SBC (ici une Radxa Q6A) — sans avoir besoin d'un clavier physique.

> **Cas d'usage typique** : configurer une Radxa Q6A (ou tout autre SBC/PC) qui démarre sans clavier, en se connectant à l'Arduino GIGA R1 WiFi depuis son téléphone ou PC via le réseau local.

---

## Matériel requis

| Composant | Rôle |
|---|---|
| Arduino GIGA R1 WiFi | Serveur web + émulation clavier USB HID |
| Arduino GIGA Display Shield (optionnel) | Affiche l'IP et les boutons de mode |
| Câble USB-C | Relie l'Arduino au port USB du SBC cible |
| SBC cible (ex: Radxa Q6A) | Reçoit les frappes clavier |

---

## Fonctionnalités

- 🌐 **Page web embarquée** accessible depuis tout navigateur sur le réseau local
- ⌨️ **Émulation USB HID** — l'Arduino se présente comme un vrai clavier physique
- 🇫🇷 **Mode AZERTY FR** avec table de conversion des positions de touches HID
- 🇺🇸 **Mode QWERTY EN** (envoi direct)
- 🖥️ **Affichage sur GIGA Display Shield** : IP, mode actif, boutons tactiles AZERTY/QWERTY
- 🔄 **Changement de mode synchronisé** : depuis l'écran tactile OU depuis la page web
- 🎮 Touches spéciales : Enter, Tab, Backspace, Del, Esc, flèches, Ctrl+C/Z/D

---

## Installation

### 1. Bibliothèques

Installer via le **Gestionnaire de bibliothèques** de l'IDE Arduino :
- `Arduino_GigaDisplayTouch` (par Arduino)

Les bibliothèques suivantes sont déjà incluses dans le core **Arduino Mbed GIGA** :
- `PluggableUSBHID` + `USBKeyboard`
- `Arduino_H7_Video` + `ArduinoGraphics`

### 2. Configuration

Ouvrir `GIGA_R1_Virtual_Keyboard/GIGA_R1_Virtual_Keyboard.ino` et modifier :

```cpp
const char* ssid     = "VOTRE_SSID";
const char* password = "VOTRE_MOT_DE_PASSE";
```

### 3. Téléversement

1. Sélectionner la carte **Arduino GIGA R1 WiFi** dans l'IDE
2. Téléverser le sketch
3. Ouvrir le **Moniteur Série** (115200 baud) pour voir l'adresse IP affectée

### 4. Connexion

1. Brancher le port **USB-C** de l'Arduino GIGA R1 sur un port USB-A du SBC cible
2. Depuis votre navigateur, ouvrir `http://<IP_Arduino>`
3. Taper du texte et appuyer sur **Envoyer** (ou Entrée)

---

## Disposition AZERTY — Note technique

Les codes HID USB représentent des **positions physiques de touches**, pas des caractères.
En mode AZERTY, l'Arduino envoie le keycode correspondant à la position physique correcte,
et le SBC cible (configuré en FR) interprète la position comme le bon caractère.

Exemple : pour envoyer `q`, l'Arduino envoie le keycode `0x04` (position "A" sur QWERTY physique),
que la Radxa avec un layout AZERTY lit comme `q`.

---

## Compatibilité

- ✅ Testé sur **Arduino GIGA R1 WiFi** (core `mbed_giga`)
- ❌ Incompatible avec les cartes AVR/SAMD (UNO, Mega, Leonardo...) — la bibliothèque `Keyboard.h` standard ne fonctionne pas sur GIGA
- Le GIGA Display Shield est **optionnel** — retirer les includes `Arduino_H7_Video` et `ArduinoGraphics` si non utilisé

---

## Licence

[Creative Commons Attribution-NonCommercial 4.0 International](https://creativecommons.org/licenses/by-nc/4.0/)

© 2026 Kyuwei (Romain Frippiat) — Libre d'utilisation et de modification à des fins **non commerciales**, avec attribution.
