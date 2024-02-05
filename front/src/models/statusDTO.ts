export default class Status {
  status: string = ''
  success: boolean = false
  data: Data = new Data()
}

export class Data {
  main_level: number = 0
  center_level: number = 0
  rear_level: number = 0
  sub_level: number = 0
  current_input: number = 0
  current_fx: number = 0
  muted: boolean = false
  decode_mode: boolean = true
  fx_input_1: number = 0
  fx_input_2: number = 0
  fx_input_3: number = 0
  fx_input_4: number = 0
  fx_input_5: number = 0
  fx_input_aux: number = 0
  spdif_status: number = 0
  signal_status: number = 0
  stby: number = 0
  auto_stby: number = 0
}
