/*
 * sx1261_WuR.c
 *
 *  Created on: 17 avr. 2023
 *      Author: PC_SIGMACOM
 */

#include <sx1261_WuR.h>
#include "sx126x.h"
#include <stdio.h>
#include "esp_log.h"


#define TAG "WUR"


void Sx1261_Standby(context_t *context)
{
	sx126x_set_standby(&context, SX126X_STANDBY_CFG_XOSC);
}

void Sx1261_ContinousWave(context_t *context)
{
	sx126x_set_tx_cw(&context);
}

void Sx1261_InfinitePreamble(context_t *context)
{
	sx126x_set_tx_infinite_preamble(&context);
}

void Sx1261_WurMode(context_t *context, uint32_t sleepTime, uint32_t rxTime)
{
	sx126x_set_rx_duty_cycle(&context, rxTime, sleepTime);
}

void Sx1261_RstWurFlag(context_t *context)
{
	sx126x_clear_irq_status(&context, SX126X_IRQ_SYNC_WORD_VALID);
}

void Sx1261_InitParam_WUR(context_t *context, uint8_t* syncword)
{

	context->sync_word[0] = syncword[0];
	context->sync_word[1] = syncword[1];
	context->sync_word[2] = syncword[2];
	context->sync_word[3] = syncword[3];
	context->sync_word[4] = syncword[4];
	context->sync_word[5] = syncword[5];
	context->sync_word[6] = syncword[6];
	context->sync_word[7] = syncword[7];

	context->frequency = 868000000;
	context->power = POWER;

	context->mode_parameters.br_in_bps = BR_IN_BPS;
	context->mode_parameters.fdev_in_hz = FDEV_IN_HZ;
	context->mode_parameters.pulse_shape = SX126X_GFSK_PULSE_SHAPE_OFF;
	context->mode_parameters.bw_dsb_param = BW_DSB_PARAM; // must be > br_in_bps + 2*fdev_in_hz

	context->pkt_parameters.preamble_len_in_bits = PREAMBLE_LENGTH_IN_BIT; //!< Preamble length in bits
	context->pkt_parameters.preamble_detector = PREAMBLE_DETECTION_LENGTH; //!< Preamble detection length /!\ 24bits max for 1ms RX time
	context->pkt_parameters.sync_word_len_in_bits = 8*SYNCWORD_LENGTH;	//!< Sync word length in bits (multiply by 8 because SYNCWORD_LENGTH is in bytes )
	context->pkt_parameters.address_filtering = SX126X_GFSK_ADDRESS_FILTERING_DISABLE;	//!< Address filtering configuration
	context->pkt_parameters.header_type = SX126X_GFSK_PKT_FIX_LEN;	//!< Header type
	context->pkt_parameters.pld_len_in_bytes = 10;	//!< Payload length in bytes
	context->pkt_parameters.crc_type = SX126X_GFSK_CRC_OFF;	//!< CRC type configuration
	context->pkt_parameters.dc_free = SX126X_GFSK_DC_FREE_OFF;	//!< Whitening configuration
}

void set_sync_word(context_t *context, const uint8_t* sync_word, const uint8_t sync_word_length)
{
	sx126x_set_gfsk_sync_word(context, sync_word, sync_word_length); 
}


void Sx1261_InitFonction_WUR(context_t *context)
{
	//all this steps are described in the datasheet p98
	(void) sx126x_set_standby(context, SX126X_STANDBY_CFG_XOSC);
	(void) sx126x_set_pkt_type(context, SX126X_PKT_TYPE_GFSK);
	(void) sx126x_set_rf_freq(context, context->frequency);
	(void) sx126x_set_buffer_base_address(context, 0x00, 0x00);
	(void) sx126x_set_tx_params(context, context->power, RAMP_TIME);
	(void) sx126x_set_gfsk_mod_params(context, &(context->mode_parameters));
	(void) sx126x_set_gfsk_pkt_params(context, &(context->pkt_parameters));
	(void) sx126x_set_gfsk_sync_word(context, context->sync_word, context->pkt_parameters.sync_word_len_in_bits/8); //
	(void) sx126x_set_reg_mode(context, SX126X_REG_MODE_DCDC); // SX126X_REG_MODE_DCDC
	(void) sx126x_set_rx_tx_fallback_mode(context, SX126X_FALLBACK_STDBY_XOSC); // Return to standby xosc after RX or TX mode.
	(void) sx126x_set_dio_irq_params(context, SX126X_IRQ_SYNC_WORD_VALID, SX126X_IRQ_SYNC_WORD_VALID, SX126X_IRQ_NONE, SX126X_IRQ_NONE);
	
}


void Sx1261_Send_WuR_Signal( context_t *context, uint8_t* data, uint8_t length )
{
	uint8_t n_length = length;
	if (n_length > 255) {
		n_length = 255;
	}
	n_length = length; // length in bytes (for Payload successfully sent)

	ESP_LOGI(TAG, "Payload to send:");
	for (int i = 0; i < length; i++) {
		ESP_LOGI(TAG, "Byte[%d] = 0x%02X", i, data[i]);
	}


	(void) sx126x_set_dio_irq_params(context, SX126X_IRQ_ALL, SX126X_IRQ_ALL, SX126X_IRQ_NONE, SX126X_IRQ_NONE);
    sx126x_clear_irq_status(context, SX126X_IRQ_ALL);
	sx126x_write_buffer( context, 0,  data, length );

	delay(10);
	sx126x_set_tx(context, 2000);
}




//----------------------------EOF----------------------------