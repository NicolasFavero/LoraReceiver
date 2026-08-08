#pragma once

// Selecao da PCB alvo -- definida via build_flags no platformio.ini
// (um environment por placa: [env:pcb-autonomous-aircraft],
// [env:pcb-sonde], [env:pcb-lilygo-t3]). Trocar de placa e so trocar
// o environment ativo (dropdown do PlatformIO, ou
// `pio run -e pcb-sonde`) -- nao precisa mexer em include nenhum
// aqui ou no main.cpp.
//
// So pinagem mora aqui -- config de radio e de watchdog ficam em
// LoraDefaults.h e WatchdogConfig.h (nao sao "pinos", nao faz
// sentido morar num arquivo chamado Pins.h).
#if (defined(PCB_AUTONOMOUS_AIRCRAFT) + defined(PCB_SONDE) + defined(PCB_LILYGO_T3)) > 1
    #error "Defina so UMA das PCB_AUTONOMOUS_AIRCRAFT / PCB_SONDE / PCB_LILYGO_T3 em build_flags."
#elif defined(PCB_AUTONOMOUS_AIRCRAFT)
    #include "PinsAutonomousAircraft.h"
#elif defined(PCB_SONDE)
    #include "PinsSonde.h"
#elif defined(PCB_LILYGO_T3)
    #include "PinsLilyGoT3.h"
#else
    #error "Nenhuma PCB selecionada -- defina PCB_AUTONOMOUS_AIRCRAFT, PCB_SONDE ou PCB_LILYGO_T3 em build_flags (platformio.ini)."
#endif
