//  Import Libraries
use core::{      //  Rust Core Library
    fmt::Write,  //  String Formatting    
};
use sx126x::{    //  SX1262 Library
    conf::Config as LoRaConfig,  //  LoRa Configuration
    op::*,       //  LoRa Operations
    SX126x,      //  SX1262 Driver
};
use crate::{     //  Local Library
    puts,        //  Print to serial console
    String,      //  String Library
};

/// Test the SX1262 Driver by reading SX1262 Register 8.
/// Based on https://github.com/tweedegolf/sx126x-rs/blob/master/examples/stm32f103-ping-pong.rs
pub fn test_sx1262() {
    puts("test_sx1262");

    /*
    let lora_pins = (
        lora_nss,    // D7
        lora_nreset, // A0
        lora_busy,   // D4
        lora_ant,    // D8
        lora_dio1,   // D6
    );

    //  Init a busy-waiting delay
    let delay = &mut Delay::new();

    //  Init LoRa modem
    let conf = build_config();
    let mut lora = SX126x::new(lora_pins);
    lora.init(spi1, delay, conf)
        .expect("sx1262 init failed");

    //  Read register 8
    let mut result: [ u8; 1 ] = [ 0; 1 ];
    lora.read_register(spi1, delay, 8, &mut result)
        .expect("sx1262 read register failed");

    //  Show the received register value
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
