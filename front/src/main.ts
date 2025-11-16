/**
 * main.ts
 *
 * Bootstraps Vuetify and other plugins then mounts the App`
 */

// Plugins
import { registerPlugins } from '@/plugins'

// Components
import App from './App.vue'

// Store
import { createPinia } from 'pinia'

// Composables
import { createApp } from 'vue'

const pinia = createPinia()
const app = createApp(App)

app.use(pinia)
registerPlugins(app)

app.mount('#app')
