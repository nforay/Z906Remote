export interface IStatus {
  main_level: number
  center_level: number
  rear_level: number
  sub_level: number
  current_input: number
  current_fx: number
  muted: boolean
  decode_mode: boolean
  fx_input_1: number
  fx_input_2: number
  fx_input_3: number
  fx_input_4: number
  fx_input_5: number
  fx_input_aux: number
  spdif_status: number
  signal_status: number
  stby: number
  auto_stby: number
}

export class StatusDTO implements IStatus {
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

export default class Status extends StatusDTO {
  constructor(dto: StatusDTO) {
    super()
    Object.assign(this, dto)
  }

  get currentFx(): number {
    return [
      this.fx_input_1,
      this.fx_input_2,
      this.fx_input_3,
      this.fx_input_4,
      this.fx_input_5,
      this.fx_input_aux
    ][this.current_input] ?? 0
  }
}
