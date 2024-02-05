<template>
  <router-view />
</template>

<script lang="ts" setup>
import axios from 'axios'

const endpoint = import.meta.env.VITE_API_ENDPOINT

//set env VITE_API_ENDPOINT to host the front-end on another device
if (typeof endpoint !== 'undefined') {
  axios.defaults.baseURL = 'http://' + endpoint
}
axios.defaults.headers['Access-Control-Allow-Origin'] = '*'
</script>
