<template>
  <v-container>
    <v-row>
      <v-col cols="12">
        <v-stepper v-model="e6" vertical>
          <v-stepper-step :complete="e6 > 1" step="1">Welcome</v-stepper-step>

          <v-stepper-content step="1">
            <v-card color="grey lighten-1" class="mb-12" height="200px">
              <v-card-title>How does it work?</v-card-title>
              <v-card-text>
                The wizard will guide you through a short calibration process. You'll need:
                <ul>
                  <li>A glass of water, large enough to fit the sensor up to the white line</li>
                  <li>A fresh set of AA batteries</li>
                </ul>
                <p>
                We will take two readings from the sensor, one when it is completely dry and one
                when it is submerged in water. With this values we can calibrate the sensor readings.
                </p> 
              </v-card-text>
            </v-card>
            <v-btn color="primary" @click="e6 = 2">Continue</v-btn>
          </v-stepper-content>

          <v-stepper-step :complete="e6 > 2" step="2">Dry / Air reading</v-stepper-step>

          <v-stepper-content step="2">
            <p>
                Please make sure that the sensor is completely dry. Do not touch the sensor when taking the readings.
            </p>
            <p>
              <v-btn color="secondary"
              @click="takeDryReading">Measure</v-btn>
            </p>
            <v-btn
              color="primary"
              @click="e6 = 3"
              disabled="true">Continue</v-btn>
          </v-stepper-content>

          <v-stepper-step :complete="e6 > 3" step="3">Wet / Water reading</v-stepper-step>

          <v-stepper-content step="3">
            <v-card color="grey lighten-1" class="mb-12" height="200px"></v-card>
            <v-btn color="primary" @click="e6 = 4">Continue</v-btn>
            <v-btn text>Cancel</v-btn>
          </v-stepper-content>

          <v-stepper-step step="4">View setup instructions</v-stepper-step>
          <v-stepper-content step="4">
            <v-card color="grey lighten-1" class="mb-12" height="200px"></v-card>
            <v-btn color="primary" @click="e6 = 1">Continue</v-btn>
            <v-btn text>Cancel</v-btn>
          </v-stepper-content>
        </v-stepper>
      </v-col>
    </v-row>
  </v-container>
</template>

<script>
export default {
  name: "CalibrationWizard",

  data: () => ({
    e6: 1
  }),

  methods: {
    
  }
};
</script>
