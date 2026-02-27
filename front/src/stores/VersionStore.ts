import { defineStore } from "pinia"
import axios from "axios"

export const useVersionStore = defineStore("versionStore", {
  state: () => ({
    localVersion: "",
    latestVersion: "",
    updateAvailable: false,
    lastCheckedDay: 0
  }),

  actions: {
    getToday(): number {
      const now = new Date()
      return now.getDate()
    },

    async checkVersion(force: boolean = false) {
      try {
        const today = this.getToday();
        if (!force && this.lastCheckedDay === today) {
          return;
        }
        this.lastCheckedDay = today;
        this.updateAvailable = false;
        const res = await axios.get("/firmware")
        if (!res?.data?.version) {
          throw new Error("Invalid firmware response")
        }
        const data = res.data
        this.localVersion = data.version.current
        this.latestVersion = data.version.latest
        this.updateAvailable = this.localVersion !== this.latestVersion
      } catch (err) {
        console.error("Version Error:", err)
      }
    }
  },
  persist: true,
})
