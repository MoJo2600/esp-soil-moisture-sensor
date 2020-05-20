<template>
  <v-app>
    <v-app-bar
      app
      color="primary"
      dark
    >
      <div class="d-flex align-center">
        <v-img
          alt="Vuetify Logo"
          class="shrink mr-2"
          contain
          src="https://cdn.vuetifyjs.com/images/logos/vuetify-logo-dark.png"
          transition="scale-transition"
          width="40"
        />

        <v-img
          alt="Vuetify Name"
          class="shrink mt-1 hidden-sm-and-down"
          contain
          min-width="100"
          src="https://cdn.vuetifyjs.com/images/logos/vuetify-name-dark.png"
          width="100"
        />
      </div>

      <v-spacer></v-spacer>

        <v-icon>mdi-thermometer</v-icon>
        <span class="mr-2">{{ temperature }} °C</span>

        <v-icon :color="batteryIconColor">{{ batteryIconName }}</v-icon>
        <span class="mr-2">{{ battery }}%</span>
        
    </v-app-bar>

    <v-content>
      <CalibrationWizard/>
    </v-content>
  </v-app>
</template>

<script>
import CalibrationWizard from './components/CalibrationWizard';

export default {
  name: 'App',

  components: {
    CalibrationWizard,
  },

  data: () => ({
    battery: 0,
    temperature: 0,
    batteryIconColor: 'red',
    batteryIconName: 'mdi-battery-low'
  }),

  watch: {
    'battery': function(val) {
      if (val > 66) {
        this.batteryIconColor = '';
        this.batteryIconName = 'mdi-battery-high';
      } else if (val > 33) {
        this.batteryIconColor = 'yellow';
        this.batteryIconName = 'mdi-battery-medium';
      } else {
        this.batteryIconColor = 'red';
        this.batteryIconName = 'mdi-battery-low';
      }
    }
  },

  mounted: function() {
    this.$options.sockets.onmessage = message => {
      const jsonData = JSON.parse(message.data);
      this.battery = jsonData.battery_percent;
      this.temperature = jsonData.temperature;
    }
  }
};
</script>
