import { defineStore } from 'pinia'
import { type Ref, ref } from 'vue'
 
export interface CustomAction {
  actionName: string;
  link?: any;
}

export interface SnackbarItem {
  message: string
  status: string
  actions?: CustomAction[]
  duration?: number
}
 
export const useSnackbarStore = defineStore('SnackbarStore', () => {
  const isOpen = ref(false)
  const message = ref('')
  const status = ref('error')
  const customActions: Ref<CustomAction[]> = ref([])
  let snackbarTimeout: ReturnType<typeof setTimeout> | null = null

  const queue: SnackbarItem[] = []

  const isClosable = () => {
	  return !['success', 'info'].includes(status.value.toLowerCase())
  }

  const showNext = () => {
    if (queue.length === 0) return

    const next = queue.shift()!
    message.value = next.message
    status.value = next.status
    customActions.value = next.actions ? [...next.actions] : []
    isOpen.value = true

    const duration = next.duration ?? (!['success', 'info'].includes(next.status.toLowerCase()) ? 10000 : 2000)
    if (duration > 0) {
      if (snackbarTimeout) clearTimeout(snackbarTimeout)
      snackbarTimeout = setTimeout(() => {
        closeSnackbar()
      }, duration)
    }
  }
 
  const showSnackbar = (msg: string, st: string, optionalActions?: CustomAction[], duration?: number) => {
    queue.push({ message: msg, status: st, actions: optionalActions, duration })
    if (!isOpen.value) {
      showNext()
    }
  }
 
  function closeSnackbar() {
    isOpen.value = false
    customActions.value.splice(0, customActions.value.length)
    if (snackbarTimeout) {
      clearTimeout(snackbarTimeout)
      snackbarTimeout = null
    }
    if (queue.length > 0) {
      setTimeout(showNext, 500)
    }
  }

  return { isOpen, message, status, showSnackbar, closeSnackbar, isClosable, customActions }
})
