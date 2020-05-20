import Vue from 'vue'
import App from './App.vue'
import vuetify from './plugins/vuetify';
import VueNativeSock from 'vue-native-websocket'

Vue.config.productionTip = false

// Vue.use(VueNativeSock, 'ws://192.168.178.109:81', { format: 'json' })
Vue.use(VueNativeSock, 'ws://' + window.location.hostname +':81', { format: 'json' })

new Vue({
  vuetify,
  render: h => h(App)
}).$mount('#app')
