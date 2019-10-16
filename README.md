# HeartRate

Test on YousFace.mp4
//  Rect backgroundRect(130, 50 , 500, 250);
//  faces[0].x += 100; // only for manual foreHead detection
//  faces[0].width -= 200; // only for manual foreHead detection


For Manual foreHead detection for face.mp4 video file
//  faces[0].height = faces[0].height/3.7; // only for manualforeHead detection
//  faces[0].x = faces[0].x + 50; // only for manual foreHead detection
//  faces[0].width = faces[0].width - 70; // only for manual foreHead detection


For Manual foreHead detection for 73BPmin.mp4
background is Rect(5, 116 , 128, 200);
foreHeads[0].y += 20; // only for manualforeHead detection
foreHeads[0].height = foreHeads[0].height/3.7 - 20; // only for manualforeHead detection
foreHeads[0].x += 60; // only for manual foreHead detection
foreHeads[0].width -= 130; // only for manual foreHead detection


For Manual foreHead detection for face2.mp4 video file
//    faces[0].y -= 70;
//    faces[0].height = faces[0].height/3 ; // only for manualforeHead detection
//    faces[0].x = faces[0].x + 60; // only for manual foreHead detection
//    faces[0].width = faces[0].width -140; // only for manual foreHead detection


For printing fft in gnuplot
set xrange [0:6] // to set xrange
set xlabel "Frequencies in Hz"
set ylabel "Gain"
plot "FFTResult.txt" using 1:2  with lines
