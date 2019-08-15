# HeartRate

For Manual foreHead detection for face.mp4 video file
faces[0].height = faces[0].height/3.7; // only for manualforeHead detection
faces[0].x = faces[0].x + 50; // only for manual foreHead detection
faces[0].width = faces[0].width - 70; // only for manual foreHead detection


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
