<template>
  <v-container>
    <v-row>
      <v-col cols="12">
        <v-dialog v-model="measureDialog" hide-overlay persistent width="300">
          <v-card color="primary" dark>
            <v-card-text>
              Please stand by - Received {{currentReading}}
              <v-progress-linear
                v-model="readingSampleProgress"
                :active="showProgress"
                :indeterminate="query"
                :query="true"
                color="white"
                class="mb-0">
              </v-progress-linear>
            </v-card-text>
          </v-card>
        </v-dialog>
        <v-stepper v-model="stepper" vertical>
          <v-stepper-step :complete="stepper > 1" step="1">Welcome</v-stepper-step>

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
            <v-btn color="primary" @click="stepper = 2">Continue</v-btn>
          </v-stepper-content>

          <v-stepper-step :complete="stepper > 2" step="2">Dry / Air reading</v-stepper-step>

          <v-stepper-content step="2">
            <p>Please make sure that the sensor is completely dry. Do not touch the sensor when taking the readings.</p>
              <div class="text-center">
                <v-btn
                  :disabled="measureDialog"
                  :loading="measureDialog"
                  v-show="!showDryMeasureResult"
                  class="white--text"
                  color="purple darken-2"
                  @click="measureDry">
                  Take measurement
                </v-btn>
                <p v-show="showDryMeasureResult">
                  Dry / Air average: <b>{{ dryAverage }}</b>
                </p>
              </div>
            <v-btn color="primary" @click="stepper = 3" :disabled="dryContinueDisabled">Continue</v-btn>
          </v-stepper-content>

          <v-stepper-step :complete="stepper > 3" step="3">Moist / Water reading</v-stepper-step>

          <v-stepper-content step="3">
            <p>Now place the sensor up to the white line into a glass of tap water. Press the button when ready.</p>
              <div class="text-center">
                <v-btn
                  :disabled="measureDialog"
                  :loading="measureDialog"
                  v-show="!showMoistMeasureResult"
                  class="white--text"
                  color="purple darken-2"
                  @click="measureMoist">
                  Take measurement
                </v-btn>
                <p v-show="showMoistMeasureResult">
                  Moist / Water average: <b>{{ moistAverage }}</b>
                </p>
              </div>
            <v-btn color="primary" @click="stepper = 4" :disabled="moistContinueDisabled">Continue</v-btn>
          </v-stepper-content>

          <v-stepper-step step="4">View setup instructions</v-stepper-step>
          <v-stepper-content step="4">
            <v-card color="grey lighten-1" class="mb-12" height="200px"></v-card>
            <v-btn color="primary" @click="stepper = 1">Continue</v-btn>
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
    sampleCount: 3,
    stepper: 2,
    showDryMeasureResult: false,
    showMoistMeasureResult: false,
    measureDialog: false,
    measureTopic: '',

    dryContinueDisabled: true,
    moistContinueDisabled: true,

    dryValue: 0,
    moistValue: 0,
    query: false,
    showProgress: true,

    currentReading: 0,
    readingSampleProgress: 0,
    values: [],

    dryAverage: 0,
    moistAverage: 0
  }),

  watch: {
    measureDialog (val) {
      if (!val) return;
      this.query = true
      this.showProgress = true
      this.readingSampleProgress = 0

    //  this.measureDialog = false;
    //   TODO: Read three readings and close afterwards
    //   setTimeout(() => (this.measureDialog = false), 4000)
   },
  },

  mounted: function() {
    this.$options.sockets.onmessage = message => {
      if (this.measureDialog) {
        this.query = false;
        
        if (this.values.length <= this.sampleCount) {
          const jsonData = JSON.parse(message.data);
          this.values.push(jsonData.moisture);
          this.currentReading = jsonData.moisture;
          this.readingSampleProgress = 100 / this.sampleCount * this.values.length;

          if (this.values.length == this.sampleCount) {
            this.showProgress  = false;
            this.measureDialog = false;
            this.finishedSampling();
          }
        }
        console.log(message.data);
      }
    };
  },

  methods: {
    measureDry: function() {
      this.measureTopic='dry';
      this.measureDialog = true;
    },
    measureMoist: function() {
      this.measureTopic='moist';
      this.measureDialog=true;
    },
    finishedSampling: function() {
      const averageValue = Math.trunc(this.values.reduce((a,b) => a + b, 0) / this.values.length);
      this.values = [];
      if (this.measureTopic == 'dry') {
        this.dryAverage = averageValue;
        this.dryContinueDisabled = false;
        this.showDryMeasureResult = true;
      } else if (this.measureTopic == 'moist') {
        this.moistAverage = averageValue;
        this.moistContinueDisabled = false;
        this.showMoistMeasureResult = true;
      }
    }
  }
};
</script>
