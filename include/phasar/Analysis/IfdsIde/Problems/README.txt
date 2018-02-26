The classes IFDSProtoAnalysis and IDEProtoAnalysis can be used as prototypes
when writing a new analysis. When adding a new analysis to this framework
the following steps should be performed:

	1. Create a new directory for your analysis class (according to the naming
	 scheme)
	
	2. Create a new file for your analysis, you can copy one of the prototype
	 classes and rename it according to your analysis.
	
	3. You must register the new analysis in the following way:
		
		1. Add a one line desciption of its existance to more_help.txt
			e.g. "uninitialized variable analysis, IFDS - 'ifds_uninit'"

		2. Add you analysis to the 'DataFlowAnalysisTypeMap' in 
		 AnalysisController.cpp. Also extend the operator implementation
		 ostream& operator<<(ostream& os, const DataFlowAnalysisType& D);
		 in the same class.

		3. Include the corresponding analysis header file in AnalysisController.hh.
		
		4. Last but not least: add a case to the 'big'-switch in 
		 AnalysisController.cpp in the 'if(WPA)' branch. There you can set-up
		 required data, if not already done by this framework and start your 
		 analysis.

	4. You are now able to run your new analysis with the default function 
	 implementations. Depending on your analysis you may have to change the 
	 template arguments of the prototype classes. But for most analyses they 
	 shoud be fine. Now you can start iteratively to replace the default 
	 implementations with your own flow functions in order to express your 
	 analysis. You probably want to create a directory in 'llvm_examples/' 
	 where you can write some small example programs in order to test if the
	 analysis that you are writing does what you want it to do.

	5. Have fun!
