//! Test SX1262 Driver

//  Import Libraries
use sx126x::{     //  SX1262 Library
    conf::Config as LoRaConfig,  //  LoRa Configuration
    op::*,        //  LoRa Operations
    reg::Register,//  LoRa Registers
    SX126x,       //  SX1262 Driver
};
use crate::{      //  Local Library
    nuttx_hal,    //  NuttX Embedded HAL
};

/// TODO: Change this to your LoRa Frequency
//  const RF_FREQUENCY: u32 = 868_000_000;  //  868 MHz (EU)
//  const RF_FREQUENCY: u32 = 915_000_000;  //  915 MHz (US)
const RF_FREQUENCY: u32 = 923_000_000;  //  923 MHz (Asia)

/// SX1262 Clock Frequency
const F_XTAL: u32 = 32_000_000;  //  32 MHz

/// Test the SX1262 Driver by reading a register and sending a LoRa message.
/// Based on https://github.com/tweedegolf/sx126x-rs/blob/master/examples/stm32f103-ping-pong.rs
pub fn test_sx1262() {
    println!("test_sx1262");

    //  Open GPIO Input for SX1262 Busy Pin
    let lora_busy = nuttx_hal::InputPin::new("/dev/gpio0");

    //  Open GPIO Output for SX1262 Chip Select
    let lora_nss = nuttx_hal::OutputPin::new("/dev/gpio1");

    //  Open GPIO Interrupt for SX1262 DIO1 Pin
    let lora_dio1 = nuttx_hal::InterruptPin::new("/dev/gpio2");

    //  TODO: Open GPIO Output for SX1262 NRESET Pin
    let lora_nreset = nuttx_hal::UnusedPin::new();

    //  TODO: Open GPIO Output for SX1262 Antenna Pin
    let lora_ant = nuttx_hal::UnusedPin::new();

    //  Open SPI Bus for SX1262
    let mut spi1 = nuttx_hal::Spi::new("/dev/spitest0");

    //  Define the SX1262 Pins
    let lora_pins = (
        lora_nss,    // /dev/gpio1
        lora_nreset, // TODO
        lora_busy,   // /dev/gpio0
        lora_ant,    // TODO
        lora_dio1,   // /dev/gpio2
    );

    //  Init a busy-waiting delay
    let delay = &mut nuttx_hal::Delay::new();

    //  Init LoRa modem
    println!("Init modem...");
    let conf = build_config();
    let mut lora = SX126x::new(lora_pins);
    lora.init(&mut spi1, delay, conf)
        .expect("sx1262 init failed");

    //  Read SX1262 Register 8
    println!("Reading Register 8...");
    let mut result: [ u8; 1 ] = [ 0; 1 ];
    lora.read_register(&mut spi1, delay, 8, &mut result)
        .expect("sx1262 read register failed");

    //  Show the register value
    println!("test_sx1262: SX1262 Register 8 is 0x{:02x}", result[0]);

    //  Write SX1262 Registers to prepare for transmitting LoRa message.
    //  Based on https://gist.github.com/lupyuen/5fdede131ad0e327478994872f190668
    println!("Writing Registers...");

    //  Write Register 0x889: 0x04 (TxModulation)
    lora.write_register(&mut spi1, delay, Register::TxModulaton, &[0x04])
        .expect("write register failed");

    //  Write Register 0x8D8: 0xFE (TxClampConfig)
    lora.write_register(&mut spi1, delay, Register::TxClampConfig, &[0xFE])
        .expect("write register failed");

    //  Write Register 0x8E7: 0x38 (Over Current Protection)
    lora.write_register(&mut spi1, delay, Register::OcpConfiguration, &[0x38])
        .expect("write register failed");

    //  Write Register 0x736: 0x0D (Inverted IQ)
    lora.write_register(&mut spi1, delay, Register::IqPolaritySetup, &[0x0D])
        .expect("write register failed");

    //  Send a LoRa message
    println!("Sending LoRa message...");
    println!("Frequency: {}", RF_FREQUENCY);
    lora.write_bytes(
        &mut spi1,  //  SPI Interface
        delay,      //  Delay Interface
        b"Hello from Rust on NuttX!",  //  Payload
        0.into(),   //  Disable Transmit Timeout
        8,          //  Preamble Length
        packet::lora::LoRaCrcType::CrcOn,  //  Enable CRC
    ).expect("send failed");
}

/// Build the LoRa configuration
fn build_config() -> LoRaConfig {
    use sx126x::op::{
        irq::IrqMaskBit::*, modulation::lora::*, packet::lora::LoRaPacketParams,
        rxtx::DeviceSel::SX1262, PacketType::LoRa,
    };

    let mod_params = LoraModParams::default().into();
    let tx_params = TxParams::default()
        .set_power_dbm(14)
        .set_ramp_time(RampTime::Ramp200u);
    let pa_config = PaConfig::default()
        .set_device_sel(SX1262)  //  TODO: Select SX1261 or SX1262
        .set_pa_duty_cycle(0x04);

    let dio1_irq_mask = IrqMask::none()
        .combine(TxDone)
        .combine(Timeout)
        .combine(RxDone);

    let packet_params = LoRaPacketParams::default().into();

    let rf_freq = sx126x::calc_rf_freq(RF_FREQUENCY as f32, F_XTAL as f32);

    LoRaConfig {
        packet_type: LoRa,
        sync_word: 0x1424, // Private networks
        calib_param: CalibParam::from(0x7F),
        mod_params,
        tx_params,
        pa_config,
        packet_params: Some(packet_params),
        dio1_irq_mask,
        dio2_irq_mask: IrqMask::none(),
        dio3_irq_mask: IrqMask::none(),
        rf_frequency: RF_FREQUENCY,
        rf_freq,
    }
}

/// Read Priority Mask Register. Missing function called by sx126x crate (Arm only, not RISC-V).
/// See https://github.com/rust-embedded/cortex-m/blob/master/src/register/primask.rs#L29
#[cfg(not(target_arch = "arm"))]  //  If architecture is not Arm...
#[no_mangle]
extern "C" fn __primask_r() -> u32 { 0 }

/// Disables all interrupts. Missing function called by sx126x crate (Arm only, not RISC-V).
/// See https://github.com/rust-embedded/cortex-m/blob/master/src/interrupt.rs#L29
#[cfg(not(target_arch = "arm"))]  //  If architecture is not Arm...
#[no_mangle]
extern "C" fn __cpsid() {}

/// Enables all interrupts. Missing function called by sx126x crate (Arm only, not RISC-V).
/// See https://github.com/rust-embedded/cortex-m/blob/master/src/interrupt.rs#L39
#[cfg(not(target_arch = "arm"))]  //  If architecture is not Arm...
#[no_mangle]
extern "C" fn __cpsie() {}

/// No operation. Missing function called by sx126x crate (Arm only, not RISC-V).
/// See https://github.com/rust-embedded/cortex-m/blob/master/src/asm.rs#L35
#[cfg(not(target_arch = "arm"))]  //  If architecture is not Arm...
#[no_mangle]
extern "C" fn __nop() {}
