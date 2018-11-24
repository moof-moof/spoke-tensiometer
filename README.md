
![](img/IMG_2161b_exp2-60.JPG)

## Spoke Tensiometer

The unusual sensor around which this project is based is the stainless steel "Digital Indicator with Data Output Port" sold by [B.G. Micro](http://www.bgmicro.com/digitalcaliperwithdataoutputport.aspx). I learnt a lot about how to hack this obscure tool in David Cook's Robot Room (http://www.robotroom.com/Digital-Indicator-3.html).

![](img/tol1049.2.jpg)

Because this linear measurement tool is based on a digital caliper, it has the same “24-bit” format as the data protocol for many other oriental calipers. You can read all about that subject here: http://www.robotroom.com/Caliper-Digital-Data-Port-2.html.

Much of the code in this projects Arduino sketch is about filtering the strange spikes in the generally noisy signal from the indicator.

As the application here is a hand-held tool to directly measure the tension of bicycle spokes, you obviously have to calibrate it first against some reference spokes under controlled tension. The methodology for such a calibration is another story.

Anyway, this is what the finished tensiometer looks like.

![](img/IMG_2242_395.JPG)

The values are presented on a standard I2C driven 1602 LCD  (not shown above) in this manner:

![](img/LCD_example_x4_commented.png)

Link to the project description: https://sites.google.com/site/xnebmisc/home/tensiometer


