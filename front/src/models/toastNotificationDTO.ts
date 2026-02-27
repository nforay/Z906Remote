export interface IToastNotification {
  type: "info" | "success" | "warning" | "error"
  text: string
}

export class ToastNotificationDTO implements IToastNotification {
  type: "info" | "success" | "warning" | "error" = "info"
  text: string = ""
}

export default class ToastNotification extends ToastNotificationDTO {
  constructor(dto: ToastNotificationDTO) {
    super()
    Object.assign(this, dto)
  }
}
