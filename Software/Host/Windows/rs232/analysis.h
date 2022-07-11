struct CANFrameToReceive{
	unsigned int ID;				/* 32 bit */
	unsigned char DLC;				/*  8 bit */
	unsigned char Data[8];			/*  8 bit */
	unsigned char CTRL1;			/*  8 bit */
	unsigned char CTRL2;			/*  8 bit */
	unsigned char CTRL3;			/*  8 bit */
};

void init_CANFrameToReceive( struct CANFrameToReceive ** frame );

void init_storageFrames(struct CANFrameToReceive ** frame);

void deinit_CANframeToReceive(struct CANFrameToReceive ** frame);

void deinit_storageFrames(struct CANFrameToReceive ** frame);

void copyTwoCANFramesToReceive( struct CANFrameToReceive * frame1, struct CANFrameToReceive * frame2 );

void bufferArrayAnalysis( unsigned char * bufferArray, struct CANFrameToReceive **storageFrames_tail_ptr_ptr, unsigned int * cont_valid_msg, unsigned int * cont_not_valid_msg );