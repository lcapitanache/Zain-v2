#For backward compatibility, when using QZXing.pri, enable the complete
# library functionality

#LMLC - Habilitar en caso de uso
CONFIG += enable_encoder_qr_code \
          enable_decoder_1d_barcodes \
          enable_decoder_qr_code \
          enable_decoder_data_matrix \
          enable_decoder_aztec \
          enable_decoder_pdf17

include(./QZXing-components.pri)
