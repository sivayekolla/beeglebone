/dts-v1/;
/plugin/;

/ {
	compatible = "ti,beaglebone-black";

	part-number = "BBB-MYLED";
	version = "00A0";

	fragment@0 {
		target-path = "/";
		__overlay__ {
			myled {
				compatible = "ti,bbb-myled";
				gpios = <&gpio1 24 0>;
				status = "okay";
			};
		};
	};
};

