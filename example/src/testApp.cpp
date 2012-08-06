#include "testApp.h"



//--------------------------------------------------------------
void testApp::setup(){
	ofSetVerticalSync(true);
	ofSetFrameRate(120);
	
	scanData data;
    
	laser.connect("169.254.238.162");
	if(!laser.isConnected())
	{
		std::cout << "connection failend" << std::endl;
		return;
	}
    
	std::cout << "Connected to laser" << std::endl;
    
	std::cout << "Loging in ..." << std::endl;
	laser.login();
    
	laser.stopMeas();
    
	std::cout << "Geting scan configuration ..." << ::std::endl;
	scanCfg c = laser.getScanCfg();
    
	//std::cout << "Scanning Frequency : " << c.scaningFrequency/100.0 << "Hz AngleResolution : " << c.angleResolution/10000.0 << "deg " << std::endl;
    
	c.angleResolution = 5000;
	c.scaningFrequency = 5000;
    
	laser.setScanCfg(c);
    
	scanDataCfg cc;
	cc.deviceName = false;
	cc.encoder = 0;
	cc.outputChannel = 3;
	cc.remission = true;
	cc.resolution = 0;
	cc.position = false;
	cc.outputInterval = 1;
    
	laser.setScanDataCfg(cc);
    
	int ret = 0;
	std::cout << "Start measurements ..." << std::endl;
	laser.startMeas();
    
	std::cout << "Wait for ready status ..." << std::endl;
	ret = 0;
	while (ret != 7)
	{
		ret = laser.queryStatus();
		std::cout << "status : " << ret << std::endl;
		sleep(1);
	}
	std::cout << "Laser ready" << std::endl;
    
	std::cout << "Start continuous data transmission ..." << std::endl;
	laser.scanContinous(1);
    
	for(int i =0; i < 3; i++)
	{
		std::cout << "Receive data sample ..." << std::endl;
		laser.getData(data);
	}
    
    scanning = true;
    
    //Init bufferValues
    for( int i =0; i < NUM_TIME_SAMPLES; i++) {
        for(int j=0; j< NUM_SAMPLES; j++){
                bufferDistVals[i][j]=0;
        }
    }
    
    int currentSample =0;
    
    trackedPts.clear();
    
    blobVisible =false;
    runningBlobOpacityRampUp=false;
    runningBlobOpacityRampDown=false;
    currentBlobRingSample=0;
    blobOpacity=0;
    filterdSingleBlob.x=0;
    filterdSingleBlob.y=0;
    
    
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    
//    softcircle.allocate(256, 256, OF_IMAGE_COLOR_ALPHA);
//    softcircle.loadImage("circle_100px_fade1.tif");
    
    /*
	std::cout << "Stop continuous data transmission ..." << std::endl;
	laser.scanContinous(0);
    
	laser.stopMeas();
    
	std::cout << "Disconnect from laser" << std::endl;
	laser.disconnect();*/
}

//--------------------------------------------------------------
void testApp::update(){
    
    //If we are scanning update the data buffer with one cycle
    if(scanning){
        laser.getData(dataBuf);
        
        currentSample++;
        currentSample=currentSample%NUM_TIME_SAMPLES;
        
        
        //Put values in buffer, slow copy now replace with memcopy later
        for(int j=0; j< NUM_SAMPLES; j++){
            bufferDistVals[currentSample][j]=dataBuf.dist1[j];
        }
        
        //avg all samples
        for(int j=0; j< NUM_SAMPLES; j++){
            double tempVal =0;
            for (int i=0; i < NUM_TIME_SAMPLES; i++){
                tempVal+= bufferDistVals[0][j];
            }
            avgDistVals[j]= tempVal/NUM_TIME_SAMPLES;
        }
        
        polar2CartData();
        
        findClusterCentroids();
        
        filterSingleBlobs();
        
        filterBlobs();
       
    }
}

//--------------------------------------------------------------
void testApp::draw(){
    ofBackground(128);
    //print number of values
    //std::cout << dataBuf.dist_len1 << std::endl;
 
    //drawPolar2CartData();
    //drawTrackingAndRawData();
    
    //drawFilteredBlobs();
    
    drawSingleFilteredBlob();
    
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){

}

//--------------------------------------------------------------
void testApp::keyReleased(int key){
    if(key == 'q'){
        std::cout << "Stop continuous data transmission ..." << std::endl;
        laser.scanContinous(0);
        
        laser.stopMeas();
        
        std::cout << "Disconnect from laser" << std::endl;
        laser.disconnect();
        
        //scanning is not running
        scanning = false;
        
    }
}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}

//--------------------------------------------------------------
//Draw unfiltered data in a weird format. 
//Basically, it draws polar data is if it was cartesian.
//--------------------------------------------------------------
void testApp::drawRawData(){
    ofBeginShape();
    for(int i=0; i <dataBuf.dist_len1; i++){
        ofVertex(i, dataBuf.dist1[i]/10.0);
        
    }
    ofVertex(0, 0);
    ofEndShape();
    
}

//--------------------------------------------------------------
//Draw unfiltered data in cartesian coordinate system. 
//Convert all values from polar to cartesian.
//--------------------------------------------------------------
void testApp::drawPolar2CartData(){
    //convert from polar to cartesian
    for(int i=0; i <dataBuf.dist_len1; i++){
        
        //assume 0.5deg spacing and 45deg offset
        float deg = i*0.5-45;
        
        //calc radians from degree
        float rad = ofDegToRad(deg);
        
        //float xLoc = dataBuf.dist1[i]/5.5 * cos(rad);
        //float yLoc = dataBuf.dist1[i]/5.5 * sin(rad);
        
        float xLoc = avgDistVals[i]/5.5 * cos(rad);
        float yLoc = avgDistVals[i]/5.5 * sin(rad);
        
        
        ofCircle(xLoc+ 550, yLoc +200, 1);
        
    }
    
}

//--------------------------------------------------------------
//Draw tracked data, and centroids 
//
//--------------------------------------------------------------
void testApp::drawTrackingAndRawData(){
    
    double xOffset = 2000;
    double yOffset = 1000;
    double xScale = 0.2;
    double yScale = 0.2;
    
    //draw all pts
    ofSetColor(0,0,0);
    for (int i=0; i < NUM_SAMPLES; i++){
        ofCircle((allPts[i].x+xOffset)*xScale, (allPts[i].y +yOffset)*yScale, 1);
    }
    
    //draw tracked pts
    ofSetColor(255,255,255);
    for (int i=0; i < trackedPts.size(); i++){
        ofCircle((trackedPts[i].x+xOffset)*xScale, (trackedPts[i].y +yOffset)*yScale, 1);
    }
    
    //draw centroids
    ofSetColor(255,0,0);
    for (int i=0; i < centroids.size(); i++){
        ofCircle((centroids[i].x+xOffset)*xScale, (centroids[i].y +yOffset)*yScale, 5);
    }
    
}

//--------------------------------------------------------------
//Draw filtered blobs 
//
//--------------------------------------------------------------
void testApp::drawFilteredBlobs(){
    double xOffset = 2000;
    double yOffset = 1000;
    double xScale = 0.2;
    double yScale = 0.2;
    
    ofSetColor(0,255,0);
    if(blob1Prev!=ofPoint()){
        ofCircle((blob1Avg.x+xOffset)*xScale, (blob1Avg.y +yOffset)*yScale, 10);
        
    }
    if(blob2Prev!=ofPoint()){
        ofCircle((blob2Avg.x+xOffset)*xScale, (blob2Avg.y +yOffset)*yScale, 10);
        
    }
}


//--------------------------------------------------------------
//Draw single filtered blobs 
//USE THIS ONE
// !!!!!!!!!!!!!!!!!!!!!
//--------------------------------------------------------------
void testApp::drawSingleFilteredBlob(){
    //shifts from origin to center of tracked space, poorly TODO revise
    double xOffset = 2000; //in mm
    double yOffset = 1000;
    double xScale = 0.2;
    double yScale = 0.2;
    
    ofSetColor(0,0,blobOpacity, blobOpacity);
   
        ofCircle((filterdSingleBlob.x+xOffset)*xScale, (filterdSingleBlob.y +yOffset)*yScale, 30);
        //softcircle.draw(mouse_x - softcircle.width/2, mouse_y - softcircle.height/2);


   
    
}

//--------------------------------------------------------------
//Convert all values from polar to cartesian.
//filter data to only find those in the right spot
//--------------------------------------------------------------
void testApp::polar2CartData(){
    //convert from polar to cartesian
    trackedPts.clear();
    for(int i=0; i <dataBuf.dist_len1; i++){
        
        //assume 0.5deg spacing and 45deg offset
        float deg = i*0.5-45;
        
        //calc radians from degree
        float rad = ofDegToRad(deg);
        
        //float xLoc = dataBuf.dist1[i]/5.5 * cos(rad);
        //float yLoc = dataBuf.dist1[i]/5.5 * sin(rad);
        
        float xLoc = avgDistVals[i] * cos(rad);
        float yLoc = avgDistVals[i] * sin(rad);
        
        
        allPts[i]=ofPoint(xLoc, yLoc);
        
        //If the point is in the good area, add it to the tracked points
        if((allPts[i].x>X_MIN_BOUNDS)&&(allPts[i].x<X_MAX_BOUNDS)&&(allPts[i].y>Y_MIN_BOUNDS)&&(allPts[i].y<Y_MAX_BOUNDS)){
            trackedPts.push_back(ofPoint(allPts[i].x,allPts[i].y));
        }
        
    }
    
}

//--------------------------------------------------------------
//Find the centroids of the clusters of the tracked points
//
//--------------------------------------------------------------
void testApp::findClusterCentroids(){
    int nPoints =trackedPts.size();
    
    if(nPoints>4){
        //Need to adjust this value based on num points
        int nClusters = 1;
        
        CvMat* points = cvCreateMat(nPoints, 1, CV_32FC2);
        float* dataptr = (float*) points->data.ptr;
        for(int i = 0; i < nPoints; i++) {
            *(dataptr++) = trackedPts[i].x;
            *(dataptr++) = trackedPts[i].y;
        }
        
        //label clusters
        CvMat* labels = cvCreateMat(points->rows, 1, CV_32SC1);
        cvKMeans2(
                  points, // data to cluster
                  nClusters, // how many clusters to use
                  labels,  // resulting labels on clusters
                  cvTermCriteria( // conditions for stopping the clustering
                                 CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 10, 1.0));
        
        
        vector<int> result;
        result.resize(nPoints);
        
        int* labelptr = (int*) labels->data.ptr;
        for(int i = 0; i < labels->rows; i++)
            result[i] = *(labelptr++);
        
        cvReleaseMat(&points);
        cvReleaseMat(&labels);

        
        //find centroids of clusters
        
        clusterInfo clusters[nClusters];
        
        for(int i =0; i <nClusters; i++){
            clusters[i].xCentroid=0;
            clusters[i].yCentroid=0;
            clusters[i].xSum=0;
            clusters[i].ySum=0;
            clusters[i].numPoints=0;
            
        }
        
        //add values
        for(int i = 0; i <nPoints; i++){
                clusters[result[i]].xSum+=trackedPts[i].x;
                clusters[result[i]].ySum+=trackedPts[i].y;
                clusters[result[i]].numPoints++;
        }
        
        centroids.clear();
        for(int i =0; i <nClusters; i++){
            clusters[i].xCentroid=(clusters[i].xSum)/(clusters[i].numPoints);
            clusters[i].yCentroid=(clusters[i].ySum)/(clusters[i].numPoints);
            centroids.push_back(ofPoint(clusters[i].xCentroid, clusters[i].yCentroid));
        }
    }
    else { //clear centroids if there are no points
        centroids.clear();
    }
}
//--------------------------------------------------------------
//Filter Blobs. Only takes one centroids
//track down
//--------------------------------------------------------------
void testApp::filterSingleBlobs(){
    
    if((trackedPts.size()>5)&&(centroids.size())){ //there is a blob
        if(blobVisible==false){ //there was no blob before
            blobVisible=true;
            runningBlobOpacityRampUp=true;
            blobOpacity=10;
            
            //set all values in ring buffer to current value
            for(int i=0; i< NUM_BLOB_SAMPLES; i++){
                blobRingBuffer[i].x=centroids[0].x;
                blobRingBuffer[i].y=centroids[0].y;
                
            }
            filterdSingleBlob.x=centroids[0].x;
            filterdSingleBlob.y=centroids[0].y;
            currentBlobRingSample=0;
        }
        else {//There was a blob before, so track blob
            
            if(runningBlobOpacityRampUp==true){
                blobOpacity+=10;
                if(blobOpacity>255){
                    blobOpacity =255;
                    runningBlobOpacityRampUp=false;
                    
                }
            }
            
            //update blob samples
            currentBlobRingSample++;
            currentBlobRingSample=currentBlobRingSample%NUM_BLOB_SAMPLES;
            blobRingBuffer[currentBlobRingSample].x=centroids[0].x;
            blobRingBuffer[currentBlobRingSample].y=centroids[0].y;
            
            //avg blob samples
            filterdSingleBlob.x=0;
            filterdSingleBlob.y=0;
            for(int i=0; i< NUM_BLOB_SAMPLES; i++){
                filterdSingleBlob.x+=blobRingBuffer[i].x;
                filterdSingleBlob.y+=blobRingBuffer[i].y;
                
            }
            filterdSingleBlob.x=filterdSingleBlob.x/NUM_BLOB_SAMPLES;
            filterdSingleBlob.y=filterdSingleBlob.y/NUM_BLOB_SAMPLES;
        }
        
    }
    else{ //There is no blob
        
        if(blobVisible==true){ //there just was a blob, so transition
            cout<<"Lets be here"<<endl;
            blobVisible=false;
            //set all values in ring buffer to current value
            for(int i=0; i< NUM_BLOB_SAMPLES; i++){
                blobRingBuffer[i].x=centroids[0].x;
                blobRingBuffer[i].y=centroids[0].y;
                
            }
            
            
            runningBlobOpacityRampDown=true;
            blobOpacity =230;
            
            
        }else{
            //no blob, but still might need to change opacity
            if(runningBlobOpacityRampDown==true){
                blobOpacity-=10;
                if(blobOpacity<0){
                    blobOpacity =0;
                    runningBlobOpacityRampDown=false;
                    
                }
            }
        }
        
    }


}




//--------------------------------------------------------------
//Filter Blobs. Only takes two centroids
//This function does not quite work properly. There is a bug where it seems to keep one blob at 0,0. Should be easy to track down
//--------------------------------------------------------------
void testApp::filterBlobs(){
    //make sure there are centroids
    if(centroids.size()>0) {
        
        ofPoint currentBlob;
        
        //first check how close blobs are to eachother
        double distance = sqrt(pow((centroids[1].x-centroids[0].x),2) +pow((centroids[1].y-centroids[0].y),2));
        
        //std::cout <<distance<< std::endl;
        //if close, then reject one. Keep the closer to the sensor one
        if(distance < 350) {   
            if(centroids[1].y<centroids[0].y){
                currentBlob.x = centroids[1].x;
                currentBlob.y = centroids[1].y;
            }
            else {
                currentBlob.x = centroids[0].x;
                currentBlob.y = centroids[0].y;
            }
            //then see if this is close to prev blob1 or blob2
            double tempDist1;
            if(blob1Prev != ofPoint()) {
               tempDist1 = sqrt(pow((currentBlob.x-blob1Prev.x),2) +pow((currentBlob.y-blob1Prev.y),2));
            } else {
                tempDist1 =20000000;
            }
            
            double tempDist2; 
            if(blob2Prev != ofPoint()) {
                tempDist2= sqrt(pow((currentBlob.x-blob2Prev.x),2) +pow((currentBlob.y-blob2Prev.y),2));
            } else {
                tempDist2 =20000000;
            }
            
            //if so then avg with that one and draw
            if(tempDist2<tempDist1){
                blob2Avg.x=(currentBlob.x*2+ blob2Prev.x+blob2Avg.x)/4;
                blob2Avg.y=(currentBlob.y*2+ blob2Prev.y+blob2Avg.y)/4;
                
                blob2Prev.x=currentBlob.x; 
                blob2Prev.y=currentBlob.y;
                
                blob1Prev= ofPoint();
            }
            else{
                blob1Avg.x=(currentBlob.x*2+ blob1Prev.x+blob1Avg.x)/4;
                blob1Avg.y=(currentBlob.y*2+ blob1Prev.y+blob1Avg.y)/4;
                
                blob1Prev.x=currentBlob.x; 
                blob1Prev.y=currentBlob.y;
                
                blob2Prev= ofPoint();
            }
        }
        else{  //if not then keep both
        
            if ((blob1Prev != ofPoint())&&(blob2Prev != ofPoint())){
            //find distances between prevBlob1 and prevBlob2 and the new blobs
                double distBlobPrev1Cent1 = sqrt(pow((centroids[1].x-blob1Prev.x),2) +pow((centroids[1].y-blob1Prev.y),2));
                double distBlobPrev2Cent1 = sqrt(pow((centroids[1].x-blob2Prev.x),2) +pow((centroids[1].y-blob2Prev.y),2));
            
                double distBlobPrev1Cent2 = sqrt(pow((centroids[2].x-blob1Prev.x),2) +pow((centroids[2].y-blob1Prev.y),2));
                double distBlobPrev2Cent2 = sqrt(pow((centroids[2].x-blob2Prev.x),2) +pow((centroids[2].y-blob2Prev.y),2));    
            
                double testArray[4] = {distBlobPrev1Cent1, distBlobPrev2Cent1, distBlobPrev1Cent2, distBlobPrev2Cent2};
            
                int smallest=0;
                double small = 2000000000;
                for(int i =0; i< 4; i++){
                    if(testArray[i]<small){
                        small = testArray[i];
                        smallest = i;
                    }
                
                }
                
                switch (smallest) {
                    //distBlobPrev1Cent1 is smallest
                    case 0:
                        blob1Avg.x=(centroids[1].x*2+ blob1Prev.x+blob1Avg.x)/4;
                        blob1Avg.y=(centroids[1].y*2+ blob1Prev.y+blob1Avg.y)/4;
                        
                        blob1Prev.x=centroids[1].x; 
                        blob1Prev.y=centroids[1].y;
                        
                        blob2Avg.x=(centroids[2].x*2+ blob2Prev.x+blob2Avg.x)/4;
                        blob2Avg.y=(centroids[2].y*2+ blob2Prev.y+blob2Avg.y)/4;
                        
                        blob2Prev.x=centroids[2].x; 
                        blob2Prev.y=centroids[2].y;
                        
                        
                        break;
                        
                    //distBlobPrev2Cent1 is smallest
                    case 1:
                        blob2Avg.x=(centroids[1].x*2+ blob2Prev.x+blob2Avg.x)/4;
                        blob2Avg.y=(centroids[1].y*2+ blob2Prev.y+blob2Avg.y)/4;
                        
                        blob2Prev.x=centroids[1].x; 
                        blob2Prev.y=centroids[1].y;
                        
                        blob1Avg.x=(centroids[2].x*2+ blob1Prev.x+blob1Avg.x)/4;
                        blob1Avg.y=(centroids[2].y*2+ blob1Prev.y+blob1Avg.y)/4;
                        
                        blob1Prev.x=centroids[2].x; 
                        blob1Prev.y=centroids[2].y;
                        
                        break;
                    
                    //distBlobPrev1Cent2 is smallest
                    case 2:
                        blob2Avg.x=(centroids[1].x*2+ blob2Prev.x+blob2Avg.x)/4;
                        blob2Avg.y=(centroids[1].y*2+ blob2Prev.y+blob2Avg.y)/4;
                        
                        blob2Prev.x=centroids[1].x; 
                        blob2Prev.y=centroids[1].y;
                        
                        blob1Avg.x=(centroids[2].x*2+ blob1Prev.x+blob1Avg.x)/4;
                        blob1Avg.y=(centroids[2].y*2+ blob1Prev.y+blob1Avg.y)/4;
                        
                        blob1Prev.x=centroids[2].x; 
                        blob1Prev.y=centroids[2].y;
                        break;
                    
                    //distBlobPrev2Cent2 is smallest
                    case 3:
                        blob1Avg.x=(centroids[1].x*2+ blob1Prev.x+blob1Avg.x)/4;
                        blob1Avg.y=(centroids[1].y*2+ blob1Prev.y+blob1Avg.y)/4;
                        
                        blob1Prev.x=centroids[1].x; 
                        blob1Prev.y=centroids[1].y;
                        
                        blob2Avg.x=(centroids[2].x*2+ blob2Prev.x+blob2Avg.x)/4;
                        blob2Avg.y=(centroids[2].y*2+ blob2Prev.y+blob2Avg.y)/4;
                        
                        blob2Prev.x=centroids[2].x; 
                        blob2Prev.y=centroids[2].y;
                        break;
                        
                    default:
                        break;
                }
            }
            else{
                //there use to be only one blob
                if(blob1Prev != ofPoint()){ //Blob2Prev use to be null
                    double distBlobPrev1Cent1 = sqrt(pow((centroids[1].x-blob1Prev.x),2) +pow((centroids[1].y-blob1Prev.y),2));
                    double distBlobPrev1Cent2 = sqrt(pow((centroids[2].x-blob1Prev.x),2) +pow((centroids[2].y-blob1Prev.y),2));
                    
                    //Find the shorter distance between each centroid and prevblob1
                    if(distBlobPrev1Cent1<distBlobPrev1Cent2){
                        //centroid1 is closer to PrevBlob1
                        blob1Avg.x=(centroids[1].x*2+ blob1Prev.x+blob1Avg.x)/4;
                        blob1Avg.y=(centroids[1].y*2+ blob1Prev.y+blob1Avg.y)/4;
                        
                        blob1Prev.x=centroids[1].x; 
                        blob1Prev.y=centroids[1].y;
                        
                        blob2Avg.x=(centroids[2].x*2+ blob2Prev.x+blob2Avg.x)/4;
                        blob2Avg.y=(centroids[2].y*2+ blob2Prev.y+blob2Avg.y)/4;
                        
                        blob2Prev.x=centroids[2].x; 
                        blob2Prev.y=centroids[2].y;
                        
                    }
                    else {
                        //centroid2 is closer to PrevBlob1
                        blob2Avg.x=(centroids[1].x*2+ blob2Prev.x+blob2Avg.x)/4;
                        blob2Avg.y=(centroids[1].y*2+ blob2Prev.y+blob2Avg.y)/4;
                        
                        blob2Prev.x=centroids[1].x; 
                        blob2Prev.y=centroids[1].y;
                        
                        blob1Avg.x=(centroids[2].x*2+ blob1Prev.x+blob1Avg.x)/4;
                        blob1Avg.y=(centroids[2].y*2+ blob1Prev.y+blob1Avg.y)/4;
                        
                        blob1Prev.x=centroids[2].x; 
                        blob1Prev.y=centroids[2].y;
                    }
                    
                }else{ ////Blob1Prev use to be null
                    double distBlobPrev2Cent1 = sqrt(pow((centroids[1].x-blob2Prev.x),2) +pow((centroids[1].y-blob2Prev.y),2));
                    double distBlobPrev2Cent2 = sqrt(pow((centroids[2].x-blob2Prev.x),2) +pow((centroids[2].y-blob2Prev.y),2));
                    
                    //Find the shorter distance between each centroid and prevblob1
                    if(distBlobPrev2Cent1<distBlobPrev2Cent2){
                        //centroid1 is closer to prevBlob2
                        blob2Avg.x=(centroids[1].x*2+ blob2Prev.x+blob2Avg.x)/4;
                        blob2Avg.y=(centroids[1].y*2+ blob2Prev.y+blob2Avg.y)/4;
                        
                        blob2Prev.x=centroids[1].x; 
                        blob2Prev.y=centroids[1].y;
                        
                        blob1Avg.x=(centroids[2].x*2+ blob1Prev.x+blob1Avg.x)/4;
                        blob1Avg.y=(centroids[2].y*2+ blob1Prev.y+blob1Avg.y)/4;
                        
                        blob1Prev.x=centroids[2].x; 
                        blob1Prev.y=centroids[2].y;
                        
                    }
                    else {
                        //centroid2 is closer to prevBlob2
                        blob1Avg.x=(centroids[1].x*2+ blob1Prev.x+blob1Avg.x)/4;
                        blob1Avg.y=(centroids[1].y*2+ blob1Prev.y+blob1Avg.y)/4;
                        
                        blob1Prev.x=centroids[1].x; 
                        blob1Prev.y=centroids[1].y;
                        
                        blob2Avg.x=(centroids[2].x*2+ blob2Prev.x+blob2Avg.x)/4;
                        blob2Avg.y=(centroids[2].y*2+ blob2Prev.y+blob2Avg.y)/4;
                        
                        blob2Prev.x=centroids[2].x; 
                        blob2Prev.y=centroids[2].y;
                    }

                }
                
            }
        //the average and draw
            
        }
        
    }
    
}

//--------------------------------------------------------------
//This function is not used. 
//TODO: Remove
//--------------------------------------------------------------
vector<int> clusterPoints(vector<ofPoint> data, unsigned nClusters = 3) {
    int nPoints = data.size();
    CvMat* points = cvCreateMat(nPoints, 1, CV_32FC2);
    float* dataptr = (float*) points->data.ptr;
    for(int i = 0; i < nPoints; i++) {
        *(dataptr++) = data[i].x;
        *(dataptr++) = data[i].y;
    }
    
    CvMat* labels = cvCreateMat(points->rows, 1, CV_32SC1);
    cvKMeans2(
              points, // data to cluster
              nClusters, // how many clusters to use
              labels,  // resulting labels on clusters
              cvTermCriteria( // conditions for stopping the clustering
                             CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 10, 1.0));
    
    vector<int> result;
    result.resize(nPoints);
    
    int* labelptr = (int*) labels->data.ptr;
    for(int i = 0; i < labels->rows; i++)
        result[i] = *(labelptr++);
    
    cvReleaseMat(&points);
    cvReleaseMat(&labels);
    
    return result;
}