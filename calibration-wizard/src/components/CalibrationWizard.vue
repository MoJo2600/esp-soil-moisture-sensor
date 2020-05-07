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
                class="mb-0"
              ></v-progress-linear>
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
                @click="measureDry"
              >Take measurement</v-btn>
              <p v-show="showDryMeasureResult">
                Dry / Air average:
                <b>{{ dryAverage }}</b>
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
                @click="measureMoist"
              >Take measurement</v-btn>
              <p v-show="showMoistMeasureResult">
                Moist / Water average:
                <b>{{ moistAverage }}</b>
              </p>
            </div>
            <v-btn color="primary" @click="stepper = 4" :disabled="moistContinueDisabled">Continue</v-btn>
          </v-stepper-content>

          <v-stepper-step step="4">Done</v-stepper-step>
          <v-stepper-content step="4">
            <v-row>
              <v-col>
                Moisture reading: {{ this.moisture_raw }}
              </v-col>
              <v-col>
                Battery reading: {{ this.battery_raw }}
              </v-col>
            </v-row>
            <v-row>
              <v-col cols="6">
                <JqxGauge
                  v-if="calibrationComplete"
                  :min="minGauge"
                  :max="maxGauge"
                  ref="moistureGauge"
                  :border="{ visible: false }"
                  :value="0"
                  :colorScheme="'scheme01'"
                  :animationDuration="1500"
                  :ranges="ranges"
                  :ticksMinor="ticksMinor"
                  :ticksMajor="ticksMajor"
                  :labels="labels"
                  :styleProperty="style"
                  :caption="caption"
                  :cap="'radius: 0.04'"
                ></JqxGauge>
              </v-col>
              <v-col>
                <JqxLinearGauge ref="batteryGauge"
                  :min="0"
                  :max="1050"
                  :value="battery_raw"
                  :pointer="batteryPointer"
                  :colorScheme="'scheme01'"
                  :orientation="'vertical'"
                  :labels="batteryLabels"
                  :ticksMajor="batteryTicksMajor"
                  :ticksMinor="batteryTicksMinor"
                  :ranges="batteryRanges"
                  :background="{ visible: false }">
                </JqxLinearGauge>
              </v-col>
            </v-row>
            <v-btn color="primary" @click="stepper = 1">Save and restart to Homie</v-btn>
          </v-stepper-content>
        </v-stepper>
      </v-col>
    </v-row>
  </v-container>
</template>

<script>
import JqxGauge from "jqwidgets-scripts/jqwidgets-vue/vue_jqxgauge.vue";
import JqxLinearGauge from "jqwidgets-scripts/jqwidgets-vue/vue_jqxlineargauge.vue";

export default {
  name: "CalibrationWizard",

  components: {
    JqxGauge,
    JqxLinearGauge
  },

  data: () => ({
    // ranges: [],
    // ticksMinor: 1,
    // ticksMajor: 1,
    // lables: [],
    // style: "",
    // caption: "caption",
    calibrationComplete: true,
    sampleCount: 3,
    stepper: 4,
    showDryMeasureResult: false,
    showMoistMeasureResult: false,
    measureDialog: false,
    measureTopic: "",

    dryContinueDisabled: true,
    moistContinueDisabled: true,

    dryValue: 0,
    moistValue: 0,
    query: false,
    showProgress: true,

    currentReading: 0,
    readingSampleProgress: 0,
    values: [],

    moisture_raw: 0,
    battery_raw: 50,

    dryAverage: 830,
    moistAverage: 770,

    minGauge: 500,
    maxGauge: 900,

    ranges: [],

    batteryPointer: { size: '8%' },
    batteryLabels: { interval: 100 },
    batteryTicksMinor: { size: '5%', interval: 25, style: { 'stroke-width': 1, stroke: '#aaaaaa' } },
    batteryTicksMajor: { size: '10%', interval: 100 },
    batteryRanges: [
        { startValue: 0, endValue: 500, style: { fill: '#FF4800', stroke: '#FF4800' } },
        { startValue: 500, endValue: 800, style: { fill: '#FFA200', stroke: '#FFA200' } },
        { startValue: 800, endValue: 1024, style: { fill: '#228B22', stroke: '#228B22' } }
    ]

  }),

  beforeCreate: function () {
    this.ticksMinor = { interval: 5, size: '5%' };
    this.ticksMajor = { interval: 25, size: '10%' };
    this.labels = { visible: true, position: 'inside', interval: 50 };
    this.style = { stroke: '#ffffff', 'stroke-width': '1px', fill: '#eeeee' };
    this.caption = { offset: [0, -25], value: 'Moisture', position: 'bottom' };
  },

  watch: {
    measureDialog(val) {
      if (!val) return;
      this.query = true;
      this.showProgress = true;
      this.readingSampleProgress = 0;

      //  this.measureDialog = false;
      //   TODO: Read three readings and close afterwards
      //   setTimeout(() => (this.measureDialog = false), 4000)
    },
    stepper(val) {
      if (!val) return;
      if (val == 4) {
        // To display nice gauge labels
        this.minGauge = Math.floor(this.moistAverage/10)*10;
        this.maxGauge = Math.ceil(this.dryAverage/10)*10;

        const steps = Math.round((this.maxGauge - this.minGauge) / 3) 

        this.ranges =
        [
            { startValue: this.minGauge, endValue: this.minGauge+steps, style: { fill: '#db5016', stroke: '#db5016' }, startDistance: '5%', endDistance: '5%', endWidth: 13, startWidth: 13 },
            { startValue: this.minGauge+steps, endValue: this.maxGauge-steps, style: { fill: '#228B22', stroke: '#228B22' }, startDistance: '5%', endDistance: '5%', endWidth: 13, startWidth: 13 },
            { startValue: this.maxGauge-steps, endValue: this.maxGauge, style: { fill: '#db5016', stroke: '#db5016' }, startDistance: '5%', endDistance: '5%', endWidth: 13, startWidth: 13 },
        ];

        this.calibrationComplete = true;
      }
    }
  },

  mounted: function() {
    this.$options.sockets.onmessage = message => {
      const jsonData = JSON.parse(message.data);

      this.moisture_raw = jsonData.moisture_raw;
      this.battery_raw = jsonData.battery_raw;

      if (this.$refs.moistureGauge) {
        this.$refs.moistureGauge.value = jsonData.moisture_raw;
        // this.$refs.moistureGauge.caption = { offset: [0, -25], value: `Moisture: ${jsonData.moisture_raw}`, position: 'bottom' };

      }

      if (this.$refs.moistureGauge) {
        this.$refs.batteryGauge.value = jsonData.battery_raw;
      }


      if (this.measureDialog) {
        this.query = false;

        if (this.values.length <= this.sampleCount) {
          this.values.push(jsonData.moisture_raw);
          this.currentReading = jsonData.moisture_raw;
          this.readingSampleProgress =
            (100 / this.sampleCount) * this.values.length;

          if (this.values.length == this.sampleCount) {
            this.showProgress = false;
            this.measureDialog = false;
            this.finishedSampling();
          }
        }
      }
      console.log(message.data);
    };
  },

  methods: {
    measureDry: function() {
      this.measureTopic = "dry";
      this.measureDialog = true;
    },
    measureMoist: function() {
      this.measureTopic = "moist";
      this.measureDialog = true;
    },
    finishedSampling: function() {
      const averageValue = Math.trunc(
        this.values.reduce((a, b) => a + b, 0) / this.values.length
      );
      this.values = [];
      if (this.measureTopic == "dry") {
        this.dryAverage = averageValue;
        this.dryContinueDisabled = false;
        this.showDryMeasureResult = true;
      } else if (this.measureTopic == "moist") {
        this.moistAverage = averageValue;
        this.moistContinueDisabled = false;
        this.showMoistMeasureResult = true;
      }
    }
  }
};
</script>
<style>

    .gaugeValue {
        background-image: -webkit-gradient(linear, 50% 0%, 50% 100%, color-stop(0%, #fafafa), color-stop(100%, #f3f3f3));
        background-image: -webkit-linear-gradient(#fafafa, #f3f3f3);
        background-image: -moz-linear-gradient(#fafafa, #f3f3f3);
        background-image: -o-linear-gradient(#fafafa, #f3f3f3);
        background-image: linear-gradient(#fafafa, #f3f3f3);
        -webkit-border-radius: 3px;
        -moz-border-radius: 3px;
        border-radius: 3px;
        -webkit-box-shadow: 0 0 50px rgba(0, 0, 0, 0.2);
        -moz-box-shadow: 0 0 50px rgba(0, 0, 0, 0.2);
        box-shadow: 0 0 50px rgba(0, 0, 0, 0.2);
        padding: 10px;
        position: absolute;
        top: 235px;
        left: 132px;
        font-family: Sans-Serif;
        text-align: center;
        font-size: 17px;
        width: 70px;
    }
</style>