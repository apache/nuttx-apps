//  Import Libraries
use core::{       //  Rust Core Library
    fmt::Write,   //  String Formatting    
};
use sx126x::{     //  SX1262 Library
    conf::Config as LoRaConfig,  //  LoRa Configuration
    op::*,        //  LoRa Operations
    SX126x,       //  SX1262 Driver
};
use crate::nuttx_hal::{  //  NuttX HAL
    NxInputPin,      //  GPIO Input
    NxOutputPin,     //  GPIO Output
    NxInterruptPin,  //  GPIO Interrupt
    NxUnusedPin,     //  Unused Pin
    NxSpi,           //  SPI Bus
};
use crate::{      //  Local Library
    puts,         //  Print to serial console
    String,       //  String Library
};

/// Test the SX1262 Driver by reading SX1262 Register 8.
/// Based on https://github.com/tweedegolf/sx126x-rs/blob/master/examples/stm32f103-ping-pong.rs
pub fn test_sx1262() {
    puts("test_sx1262");

    //  Open GPIO Input for SX1262 Busy Pin
    let mut lora_busy = NxInputPin::new(b"/dev/gpio0\0".as_ptr());

    //  Open GPIO Output for SX1262 Chip Select
    let mut lora_nss = NxOutputPin::new(b"/dev/gpio1\0".as_ptr());

    //  Open GPIO Interrupt for SX1262 DIO1 Pin
    let mut lora_dio1 = NxInterruptPin::new(b"/dev/gpio2\0".as_ptr());

    //  TODO: Open GPIO Output for SX1262 NRESET Pin
    let mut lora_nreset = NxUnusedPin::new();

    //  TODO: Open GPIO Output for SX1262 Antenna Pin
    let mut lora_ant = NxUnusedPin::new();

    //  Open SPI Bus for SX1262
    let mut spi1 = NxSpi::new(b"/dev/spitest0\0".as_ptr());

    //  Define the SX1262 Pins
    let lora_pins = (
        lora_nss,    // /dev/gpio1
        lora_nreset, // TODO
        lora_busy,   // /dev/gpio0
        lora_ant,    // TODO
        lora_dio1,   // /dev/gpio2
    );

    /*
    //  Init a busy-waiting delay
    let delay = &mut Delay::new();

    //  Init LoRa modem
    let conf = build_config();
    let mut lora = SX126x::new(lora_pins);
    lora.init(spi1, delay, conf)
        .expect("sx1262 init failed");

    //  Read SX1262 Register 8
    let mut result: [ u8; 1 ] = [ 0; 1 ];
    lora.read_register(spi1, delay, 8, &mut result)
        .expect("sx1262 read register failed");

    //  Show the register value
    let mut buf = String::new();
    write!(buf, "SX1262 Register 8 is 0x{:02x}", result[0])
        .expect("buf overflow");
    puts(&buf);
    */
}

/// Build the LoRa configuration
fn build_config() -> LoRaConfig {
    use sx126x::op::{
        irq::IrqMaskBit::*, modulation::lora::*, packet::lora::LoRaPacketParams,
        rxtx::DeviceSel::SX1261, PacketType::LoRa,
    };

    let mod_params = LoraModParams::default().into();
    let tx_params = TxParams::default()
        .set_power_dbm(14)
        .set_ramp_time(RampTime::Ramp200u);
    let pa_config = PaConfig::default()
        .set_device_sel(SX1261)
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

/// LoRa Frequency
const RF_FREQUENCY: u32 = 868_000_000; // 868MHz (EU)

/// SX1262 Clock Frequency
const F_XTAL: u32 = 32_000_000; // 32MHz
