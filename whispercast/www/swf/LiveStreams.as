/*
 * Sample from Adobe, modified by Whispersoft s.r.l.
 *
 * Original copyright notice:
 *
 * (C) Copyright 2007 Adobe Systems Incorporated. All Rights Reserved.
 *
 * NOTICE:  Adobe permits you to use, modify, and distribute this file in accordance with the 
 * terms of the Adobe license agreement accompanying it.  If you have received this file from a 
 * source other than Adobe, then your use, modification, or distribution of it requires the prior 
 * written permission of Adobe. 
 * THIS CODE AND INFORMATION IS PROVIDED "AS-IS" WITHOUT WARRANTY OF
 * ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 *  THIS CODE IS NOT SUPPORTED BY Adobe Systems Incorporated.
 *
 */

package {
    import flash.display.MovieClip;
	
    import flash.net.NetConnection;
    import flash.events.NetStatusEvent;
    import flash.events.ActivityEvent;
	import flash.events.MouseEvent;
    
    import flash.net.NetStream;
    import flash.media.Video;
    import flash.media.Microphone;
    import flash.media.Camera;
	import flash.display.LoaderInfo;
    
    public class LiveStreams extends MovieClip
    {
        var nc:NetConnection;
        var ns:NetStream;
        var video:Video;
        var camera:Camera;
        var mic:Microphone;
        var command:String;
        public function LiveStreams() {
			startBtn.addEventListener(MouseEvent.CLICK, startHandler);
			startSaveBtn.addEventListener(MouseEvent.CLICK, startSaveHandler);
			startAppendBtn.addEventListener(MouseEvent.CLICK, startAppendHandler);
			stopBtn.addEventListener(MouseEvent.CLICK, stopHandler);
			
        }
		private function startHandler(event:MouseEvent):void {
			startStreaming("live");
		}
		private function startSaveHandler(event:MouseEvent):void {
			startStreaming("record");
		}
		private function startAppendHandler(event:MouseEvent):void {
			startStreaming("append");
		}		
		private function startStreaming(cmd:String):void {
			command = cmd;
			nc = new NetConnection();
            nc.addEventListener(NetStatusEvent.NET_STATUS, netStatusHandler);
			var query_strings:Object = this.loaderInfo.parameters;
			var host:String = "192.168.2.18";  // "192.168.2.167"; //  // "localhost";
			var port:String = "8935";  // "1935";			
			if ( query_strings != null ) {
				if ( query_strings.host != null ) {
					host = query_strings.host;
				}
				if ( query_strings.port != null ) {
					port = query_strings.port;
				}
			}
			var tcUrl:String = "rtmp://" + host + ":" + port + "/live";
			trace("Connecting to: " + tcUrl);
            nc.connect(tcUrl);
			stopBtn.visible = true;
			startBtn.visible = false;
			startSaveBtn.visible = false;
			startAppendBtn.visible = false;
		}
		private function stopHandler(event:MouseEvent):void {
			stopStreaming();
		}
		private function stopStreaming():void {
			nc.close();
			video.clear();
			stopBtn.visible = false;
			startBtn.visible = true;
			startSaveBtn.visible = true;
			startAppendBtn.visible = true;
			video.visible = false;
			trace("Closed net connection");
		}		
        private function netStatusHandler(event:NetStatusEvent):void {
            trace(event.info.code);			
            switch (event.info.code)
            {
                case "NetConnection.Connect.Success":
	                publishLiveStream();
	                break;
				case "NetConnection.Connect.Closed":
					stopStreaming();
					break;
	        }
        }
       	private function activityHandler(event:ActivityEvent):void {
		    trace("activityHandler: " + event);
		    trace("activating: " + event.activating);
	    } 
        
		//  Create a live stream, attach the camera and microphone, and
		//  publish on the provided server
        private function publishLiveStream():void {
		    ns = new NetStream(nc);
		    ns.addEventListener(NetStatusEvent.NET_STATUS, netStatusHandler);
		    
		    camera = Camera.getCamera();
		    mic = Microphone.getMicrophone();
		    if (camera != null){				
				trace("BW: " + camera.bandwidth + " Quality: " + camera.quality);
				camera.setQuality(int(bandwidthInput.text), 
								  int(videoQualityInput.text));
				camera.addEventListener(ActivityEvent.ACTIVITY, activityHandler);
				if ( video == null ) {
					video = new Video();
				}
				video.visible = true;
				video.attachCamera(camera);
				ns.attachCamera(camera);
                addChild(video);   // display the video
			} else {
				trace("Cannot find a video camera !");
			}
			if (mic != null) {
		  		mic.codec = "Speex";
				mic.setSilenceLevel(0);
				mic.setUseEchoSuppression(true);
				mic.encodeQuality = int(audioQualityInput.text);
				mic.addEventListener(ActivityEvent.ACTIVITY, activityHandler);
			    ns.attachAudio(mic);
			} else {
				trace("Cannot find a microphone");
			}
			
			if (camera != null || mic != null){
				// start publishing
			    // triggers NetStream.Publish.Start
				var query_strings:Object = this.loaderInfo.parameters;
				var stream:String = "livestream/live1";
				if ( query_strings != null ) {
					if ( query_strings.stream != null ) {
						stream = query_strings.stream;
					}
				}
				ns.publish(stream, command); 
		    } else {
			    trace("Please check your camera and microphone");
		    }
	    }  
    }	
}

