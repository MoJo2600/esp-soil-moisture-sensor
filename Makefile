SHELL := /bin/bash
.DEFAULT_GOAL := help

.PHONY: help
help:  ## help target to show available commands with information
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) |  awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

.PHONY: all
all: build-calibration-frontend build-firmware upload-firmware upload-fs

.PHONY: build-calibration-frontend
build-calibration-frontend:  ## build and start backend
	cd calibration-wizard && ./build.sh && cd ..

.PHONY: build-firmware
build-firmware:
	platformio run

.PHONY: upload-firmware
upload-firmware:
	platformio run -t upload

.PHONY: upload-fs
upload-fs:
	platformio run -t uploadfs

.PHONY: start-calibration-frontend
start-wizard:  ## start backend
	docker-compose up

