menu "BLE BAS Service"

config CONTROLLER_BAS_ENABLE
	bool
	help
	  This option enables battery service.

if CONTROLLER_BAS_ENABLE

module = CONTROLLER_BAS
module-str = battery service
source "subsys/logging/Kconfig.template.log_config"

endif

endmenu