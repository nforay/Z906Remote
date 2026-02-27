export interface ILogMessage {
  level: "log" | "info" | "warn" | "error"
  msg: string
}

export class LogMessageDTO implements ILogMessage {
  level: "log" | "info" | "warn" | "error" = "log"
  msg: string = ""
}

export default class LogMessage extends LogMessageDTO {
  constructor(dto: LogMessageDTO) {
    super()
    Object.assign(this, dto)
  }

  logToConsole() {
    switch (this.level) {
      case "info":
        console.info(this.msg);
        break;
      case "warn":
        console.warn(this.msg);
        break;
      case "error":
        console.error(this.msg);
        break;
      default:
        console.log(this.msg);
        break;
    }
  }
}
