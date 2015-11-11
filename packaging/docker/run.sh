#!/bin/bash

spectrum2_manager start
sleep 2
tail -f /var/log/spectrum2/*
