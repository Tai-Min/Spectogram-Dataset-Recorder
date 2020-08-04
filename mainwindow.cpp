#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <qfiledialog.h>
#include <qaudiodeviceinfo.h>
#include <QMessageBox>
#include <QTextStream>
#include <QPixmap>

#include <algorithm>
#include <complex>

#include "thirdparty/cnpy/cnpy.h"

#include <iostream>
using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    startSound(":/start.wav"),
    stopSound(":/stop.wav")
{

    ui->setupUi(this);
    this->setFixedSize(QSize(500,660));

    ui->spectogramLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    QDir dir;
    this->ui->directoryDisplay->setText(dir.absolutePath());

    const auto deviceInInfos = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    for(const QAudioDeviceInfo &info: deviceInInfos){
        this->ui->recorderDevice->addItem(info.deviceName());
    }

    const auto deviceOutInfos = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    for(const QAudioDeviceInfo &info: deviceOutInfos){
        this->ui->recorderDevice->addItem(info.deviceName());
    }

    ui->stopButton->setEnabled(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

long double MainWindow::minMatrix(const AudioProcessor::vec2d & v){
    long double minVal = std::numeric_limits<long double>::max();

    for(unsigned int i = 0; i < v.size(); i++){
        for(unsigned int j = 0; j < v[i].size(); j++){
            if(v[i][j] < minVal)
                minVal = v[i][j];
        }
    }

    return minVal;
}

long double MainWindow::maxMatrix(const AudioProcessor::vec2d & v){
    long double maxVal = std::numeric_limits<long double>::min();

    for(unsigned int i = 0; i < v.size(); i++){
        for(unsigned int j = 0; j < v[i].size(); j++){
            if(v[i][j] > maxVal)
                maxVal = v[i][j];
        }
    }

    return maxVal;
}

QImage MainWindow::spectogramToImg(const AudioProcessor::vec2d & v){
    const long double minValSrc = minMatrix(v);
    const long double maxValSrc = maxMatrix(v);
    const long double colorMax = 0; // red in HSV
    const long double colorMin = 240; // dark blue in HSV
    const long double a = (colorMax - colorMin)/(maxValSrc - minValSrc);
    const long double b = colorMin - a * minValSrc;

    QImage img = QImage(v[0].size(), v.size(), QImage::Format_RGB32);

    for(unsigned int i = 0; i < v.size(); i++){
        for(unsigned int j = 0; j < v[i].size(); j++){
            QColor c = QColor::fromHsv(a*v[i][j] + b, 255, 255);
            img.setPixelColor(j, i, c);
        }
    }
    return img;
}

QAudioDeviceInfo MainWindow::getAudioDevice(QString name) {
    QAudioDeviceInfo device;
    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    for(int i = 0; i < devices.size(); ++i) {
        if(devices.at(i).deviceName() == name) {
            return devices.at(i);
        }
    }

    devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    for(int i = 0; i < devices.size(); ++i) {
        if(devices.at(i).deviceName() == name) {
            return devices.at(i);
        }
    }

    return device;
}

bool MainWindow::validateInputs(){
    // sample rate
    if(ui->sampleRateInput->text() == ""){
        QMessageBox msgBox;
        msgBox.setText("Sample rate input is empty.");
        msgBox.exec();
        return 0;
    }
    if(ui->sampleRateInput->text().toInt() < 1000){
        QMessageBox msgBox;
        msgBox.setText("Minimum sample rate is 1000.");
        msgBox.exec();
        return 0;
    }
    // channel count
    if(ui->channelCountInput->text() == ""){
        QMessageBox msgBox;
        msgBox.setText("Channel count input is empty.");
        msgBox.exec();
        return 0;
    }
    // duration
    if(ui->recordType->currentText() == "Fixed duration" && ui->durationInput->text().toInt() < 1){
        QMessageBox msgBox;
        msgBox.setText("Invalid duration input.");
        msgBox.exec();
        return 0;
    }
    // class
    if(ui->classInput->text() == ""){
        QMessageBox msgBox;
        msgBox.setText("Class input is empty.");
        msgBox.exec();
        return 0;
    }
    // pre emphasis
    if(ui->preEmphasisInput->text() == ""){
        QMessageBox msgBox;
        msgBox.setText("Pre emphasis coeff input is empty.");
        msgBox.exec();
        return 0;
    }
    bool ok;
    ui->preEmphasisInput->text().toDouble(&ok);
    if(!ok){
        QMessageBox msgBox;
        msgBox.setText("Invalid pre emphasis coeff input.\n");
        msgBox.exec();
        return 0;
    }
    if(ui->preEmphasisInput->text().toDouble() > 1 || ui->preEmphasisInput->text().toDouble() < 0){
        QMessageBox msgBox;
        msgBox.setText("Invalid pre emphasis coeff input.\n"
                       "Pre emphasis coeff should be between 0 and 1.");
        msgBox.exec();
        return 0;
    }
    // frame size
    if(ui->frameSizeInput->text() == ""){
        QMessageBox msgBox;
        msgBox.setText("Frame size input is empty.");
        msgBox.exec();
        return 0;
    }
    if(ui->frameSizeInput->text().toInt() < 1){
        QMessageBox msgBox;
        msgBox.setText("Frame size input can't be less than 1.");
        msgBox.exec();
        return 0;
    }
    // frame stride
    if(ui->frameStrideInput->text() == ""){
        QMessageBox msgBox;
        msgBox.setText("Frame stride input is empty.");
        msgBox.exec();
        return 0;
    }
    if(ui->frameStrideInput->text().toInt() < 1){
        QMessageBox msgBox;
        msgBox.setText("Frame stride input can't be less than 1.");
        msgBox.exec();
        return 0;
    }
    // fft points
    if(ui->FFTPointsInput->text() == ""){
        QMessageBox msgBox;
        msgBox.setText("FFT input is empty.");
        msgBox.exec();
        return 0;
    }
    if(ui->FFTPointsInput->text().toInt() < 1){
        QMessageBox msgBox;
        msgBox.setText("FFT input can't be less than 1.");
        msgBox.exec();
        return 0;
    }
    // filter banks
    if(ui->filterBanksInput->text() == ""){
        QMessageBox msgBox;
        msgBox.setText("Filter banks input is empty.");
        msgBox.exec();
        return 0;
    }
    if(ui->filterBanksInput->text().toInt() < 1){
        QMessageBox msgBox;
        msgBox.setText("Filter banks input can't be less than 1.");
        msgBox.exec();
        return 0;
    }
    return 1;
}

bool MainWindow::validateFormat(QAudioFormat format){
    QAudioDeviceInfo devInfo = getAudioDevice(ui->recorderDevice->currentText());
    if(!devInfo.isFormatSupported(format)){
        format = devInfo.nearestFormat(format);

        QMessageBox msgBox;
        msgBox.setText("Format (sample rate or channel count or sample size) not supported for selected device.\n\n"
                       "Supported nearest format:\n"
                       "Sample rate: " + QString::number(format.sampleRate()) + "\n"
                       "Channel count: " + QString::number(format.channelCount()) + "\n"
                       "Sample size: " + QString::number(format.sampleSize()) + "\n");
        msgBox.exec();
        return 0;
    }
    return 1;
}

void MainWindow::prepareClassFolder(){
    QDir dir;
    dir.setPath(ui->directoryDisplay->text());
    if(!dir.exists(ui->classInput->text())){
        dir.mkdir(ui->classInput->text());
    }
}

QString MainWindow::findAvailableFilename(){
    QDir dir;
    dir.setPath(ui->directoryDisplay->text());
    dir.cd(ui->classInput->text());

    QString format = ".txt";
    if(ui->fileFormat->currentText() == "Numpy array")
        format = ".npy";
    else if(ui->fileFormat->currentText() == "JPG color image" || ui->fileFormat->currentText() == "JPG grayscale image")
        format = ".jpg";

    unsigned long long fname = 1;
    while(dir.exists(QString::number(fname) + format)){
        fname++;
    }
    return QString::number(fname) + format;
}

AudioProcessor::vec2d MainWindow::processAudioBuffer(){
    const unsigned int bytesPerSample = ui->sampleSize->currentText().toInt()/8;
    const unsigned int numberOfChannels = ui->channelCountInput->text().toInt();
    const unsigned int sampleRate = ui->sampleRateInput->text().toInt();
    const long double emphasisCoeff = static_cast<long double>(ui->preEmphasisInput->text().toDouble());
    const unsigned int frameSize = ui->frameSizeInput->text().toInt();
    const unsigned int frameStride = ui->frameStrideInput->text().toInt();
    const unsigned int NFFT = ui->FFTPointsInput->text().toInt();
    const unsigned int numFilterBanks = ui->filterBanksInput->text().toInt();

    AudioProcessor::config conf = {
        bytesPerSample,
        numberOfChannels,
        sampleRate,
        emphasisCoeff,
        frameSize,
        frameStride,
        NFFT,
        numFilterBanks
    };

    AudioProcessor audioProc(conf);
    AudioProcessor::byteVec byteData(audioBuf.buffer().begin(), audioBuf.buffer().end());

    AudioProcessor::vec2d spectogram;
    if(ui->resultMatrix->currentText() == "MSFB"){
        spectogram = audioProc.MSFB(byteData);
    }
    else{
        spectogram = audioProc.MFCC(byteData);
    }

    return spectogram;
}

void MainWindow::savePlain(QString fname, QString dname, const AudioProcessor::vec2d & data){
    QFile file(dname + "/" + fname);
    if(!file.open(QIODevice::WriteOnly)){
        QMessageBox msgBox;
        msgBox.setText("Couldn't open file for save.");
        msgBox.exec();
        return;
    }

    QTextStream out(&file);
    for(unsigned int i = 0; i < data.size(); i++){
        for(unsigned int j = 0; j < data[i].size(); j++){
            out << static_cast<double>(data[i][j]) << " ";
        }
        out << '\n';
    }

    file.close();
}

void MainWindow::saveColorImg(QString fname, QString dname, const QImage & img){
    img.save(dname + "/" + fname);
}

void MainWindow::saveGrayscaleImg(QString fname, QString dname, const QImage & img){
    QImage gray = img.convertToFormat(QImage::Format_Grayscale8);
    gray.save(dname + "/" + fname);
}

void MainWindow::saveNumpy(QString fname, QString dname, const AudioProcessor::vec2d & data){
    std::string f = fname.toStdString();
    std::string d = dname.toStdString();

    AudioProcessor::vec data1d(data.size() * data[0].size());
    for(unsigned int i = 0; i < data.size(); i++){
        if(i == 0)
            std::copy(data[i].begin(), data[i].end(), data1d.begin());
        else
            std::copy(data[i].begin(), data[i].end(), data1d.begin() + i*data[i-1].size());
    }

    cnpy::npy_save(d + "/" + f, &data1d[0], {data.size(), data[0].size()}, "w");
}

void MainWindow::saveRecording(){
    AudioProcessor::vec2d spectogram = processAudioBuffer();

    // create QImage from spectogram and display it and maybe save as jpg if it's selected
    QImage spectogramImg = spectogramToImg(spectogram);
    ui->spectogramLabel->setPixmap(QPixmap::fromImage(spectogramImg.scaled(QSize(ui->spectogramLabel->width(), ui->spectogramLabel->height()))));

    prepareClassFolder();

    QString fileName = findAvailableFilename();

    QDir dir;
    dir.setPath(ui->directoryDisplay->text());
    dir.cd(ui->classInput->text());

    if(ui->fileFormat->currentText() == "Plain text"){
        savePlain(fileName, dir.path(), spectogram);
    }
    else if(ui->fileFormat->currentText() == "Numpy array"){
        saveNumpy(fileName, dir.path(), spectogram);
    }
    else if(ui->fileFormat->currentText() == "JPG color image"){
        saveColorImg(fileName, dir.path(), spectogramImg);
    }
    else if(ui->fileFormat->currentText() == "JPG grayscale image"){
        saveGrayscaleImg(fileName, dir.path(), spectogramImg);
    }
}

void MainWindow::setTimeLabel(){
    QString minStr = mins < 10 ? "0" + QString::number(mins) : QString::number(mins);
    QString secStr = secs < 10 ? "0" + QString::number(secs) : QString::number(secs);
    ui->timeLabel->setText(minStr + ":" + secStr);
}

void MainWindow::closeAndClearAudioBuffer(){
    audioBuf.buffer().clear();
    audioBuf.reset();
    audioBuf.close();
}

void MainWindow::uxRecording(){
    // disable all recording buttons for now
    ui->stopButton->setEnabled(false);
    ui->startButton->setEnabled(false);
    ui->startRepeatButton->setEnabled(false);

    // play sound
    startSound.play();
    while(!startSound.isFinished()){
        QCoreApplication::processEvents();
    }

    // disable device settings
    ui->recorderDevice->setEnabled(false);
    ui->sampleRateInput->setEnabled(false);
    ui->channelCountInput->setEnabled(false);
    ui->sampleSize->setEnabled(false);

    // disable recording settings
    ui->recordType->setEnabled(false);
    ui->durationInput->setEnabled(false);

    // disable save settings
    ui->directoryButton->setEnabled(false);
    ui->classInput->setEnabled(false);

    // disable post process settings
    ui->preEmphasisInput->setEnabled(false);
    ui->frameSizeInput->setEnabled(false);
    ui->frameStrideInput->setEnabled(false);
    ui->FFTPointsInput->setEnabled(false);
    ui->filterBanksInput->setEnabled(false);

    // enable recording buttons
    ui->stopButton->setEnabled(true);

    this->ui->timeLabel->clearFocus();
}

void MainWindow::uxIdle(){
    // disable all recording buttons for now
    ui->stopButton->setEnabled(false);
    ui->startButton->setEnabled(false);
    ui->startRepeatButton->setEnabled(false);

    // play sound
    stopSound.play();
    while(!stopSound.isFinished()){
        QCoreApplication::processEvents();
    }

    // enable device settings
    ui->recorderDevice->setEnabled(true);
    ui->sampleRateInput->setEnabled(true);
    ui->channelCountInput->setEnabled(true);
    ui->sampleSize->setEnabled(true);

    // enable recording settings
    ui->recordType->setEnabled(true);
    ui->durationInput->setEnabled(true);

    // enable save settings
    ui->directoryButton->setEnabled(true);
    ui->classInput->setEnabled(true);

    // enable post process settings
    ui->preEmphasisInput->setEnabled(true);
    ui->frameSizeInput->setEnabled(true);
    ui->frameStrideInput->setEnabled(true);
    ui->FFTPointsInput->setEnabled(true);
    ui->filterBanksInput->setEnabled(true);

    ui->stopButton->setEnabled(false);
    ui->startButton->setEnabled(false);
    ui->startRepeatButton->setEnabled(false);

    // enable recording buttons
    ui->startButton->setEnabled(true);
    if(ui->recordType->currentText() == "Fixed duration"){
        ui->startRepeatButton->setEnabled(true);
    }

    this->ui->timeLabel->clearFocus();
}

void MainWindow::startRecording(){
    if(!validateInputs())
        return;

    QAudioFormat format;
    format.setSampleRate(ui->sampleRateInput->text().toInt());
    format.setChannelCount(ui->channelCountInput->text().toInt());
    format.setSampleSize(ui->sampleSize->currentText().toInt());
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::UnSignedInt);

    if(!validateFormat(format))
        return;

    uxRecording();

    secs = 0;
    mins = 0;
    setTimeLabel();

    audioInput = new QAudioInput(format, this);
    audioInput->setNotifyInterval(1000);
    connect(audioInput, SIGNAL(notify()), this, SLOT(audioDevice_notify()));

    audioBuf.open(QIODevice::ReadWrite);

    audioInput->start(&audioBuf);
}

void MainWindow::stopRecording(){
    audioInput->stop();

    delete audioInput;

    if(ui->recordType->currentText() == "Fixed duration"){
        // fixed duration recording finished successfully
        if(secs + mins * 60 >= ui->durationInput->text().toInt()){

            saveRecording();

            closeAndClearAudioBuffer();

            // repeat recording if START REPEAT was clicked
            if(isRepeating){
                startRecording();
            }
        }
        // recording stopped using STOP button so clear stuff and finish
        else{
            isRepeating = false;

            closeAndClearAudioBuffer();
        }
    }
    // until stopped recording stopped by user so save stuff
    else{

        saveRecording();

        closeAndClearAudioBuffer();
    }

    secs = 0;
    mins = 0;
    setTimeLabel();

    if(!isRepeating){
        uxIdle();
    }
}

void MainWindow::on_recordType_currentIndexChanged(const QString &arg1)
{
    if(arg1 != "Fixed duration"){
        ui->durationInput->setEnabled(false);
        ui->startRepeatButton->setEnabled(false);
    }
    else{
        ui->durationInput->setEnabled(true);
        ui->startRepeatButton->setEnabled(true);
    }
}

void MainWindow::on_directoryButton_clicked()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::DirectoryOnly);
    QString dirName;
    if(dialog.exec()){
       dirName = dialog.selectedFiles()[0];
       this->ui->directoryDisplay->setText(dirName);
    }
}

void MainWindow::on_stopButton_clicked()
{
    stopRecording();
}

void MainWindow::on_startButton_clicked()
{
    isRepeating = false;
    startRecording();
}

void MainWindow::on_startRepeatButton_clicked()
{
    isRepeating = true;
    startRecording();
}

void MainWindow::audioDevice_notify(){

    // time input update
    secs++;
    if(secs >= 60){
        secs = 0;
        mins++;
    }
    setTimeLabel();

    // check if fixed duration timer finished and preprocess audio buffer if necessary
    if(ui->recordType->currentText() == "Fixed duration" && secs + mins * 60 >= ui->durationInput->text().toInt()){
        // make sure that there is expected number of sample in buffer
        // if there is too many samples then trim buffer
        // if there is less then append zeros
        const int sampleRate = ui->sampleRateInput->text().toInt();
        const int numChannels = ui->channelCountInput->text().toInt();
        const int bytesPerSample = ui->sampleSize->currentText().toInt()/8;
        const int recordDuration = ui->durationInput->text().toInt();
        const int expectedNumberOfSamples = sampleRate * bytesPerSample * numChannels * recordDuration;

        if(audioBuf.buffer().size() > expectedNumberOfSamples){
            const int diff = audioBuf.buffer().size() - expectedNumberOfSamples;
            audioBuf.buffer().remove(expectedNumberOfSamples, diff);
        }
        else if(audioBuf.buffer().size() < expectedNumberOfSamples){
            const int diff = expectedNumberOfSamples - audioBuf.buffer().size();
            audioBuf.buffer().append(diff, 0);
        }

        stopRecording();
    }
}
