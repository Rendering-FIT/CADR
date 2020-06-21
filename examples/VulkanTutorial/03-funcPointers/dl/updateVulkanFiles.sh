#!/bin/sh

sed 's! CADR_EXPORT ! !' < ../../../../src/CadR/VulkanLibrary.h | sed -z 's!\nnamespace CadR {\n!!' | sed -z 's!\n\n\n}!!' >./VulkanLibrary.h
sed 's!#include <CadR/VulkanLibrary.h>!#include "VulkanLibrary.h"!' < ../../../../src/CadR/VulkanLibrary.cpp | sed -z 's!using namespace CadR;\n!!' >./VulkanLibrary.cpp

sed 's! CADR_EXPORT ! !' < ../../../../src/CadR/VulkanInstance.h | sed -z 's!\nnamespace CadR {\n!!' | sed -z 's!\n\n\n}!!' >./VulkanInstance.h
sed 's!#include <CadR/VulkanInstance.h>!#include "VulkanInstance.h"!' < ../../../../src/CadR/VulkanInstance.cpp | sed 's!#include <CadR/VulkanLibrary.h>!#include "VulkanLibrary.h"!' | sed -z 's!using namespace CadR;\n!!' >./VulkanInstance.cpp

sed 's!#include <CadR/CallbackList.h>!#include "CallbackList.h"!' < ../../../../src/CadR/VulkanDevice.h | sed 's! CADR_EXPORT ! !' | sed -z 's!\nnamespace CadR {\n!!' | sed -z 's!\n\n\n}!!' >./VulkanDevice.h
sed 's!#include <CadR/VulkanDevice.h>!#include "VulkanDevice.h"!' < ../../../../src/CadR/VulkanDevice.cpp | sed 's!#include <CadR/VulkanInstance.h>!#include "VulkanInstance.h"!' | sed -z 's!\nusing namespace CadR;\n\n!\n\n!' >./VulkanDevice.cpp

sed -z 's!\nnamespace CadR {\n!!' < ../../../../src/CadR/CallbackList.h | sed -z 's!\n\n\n}!!' > ./CallbackList.h
