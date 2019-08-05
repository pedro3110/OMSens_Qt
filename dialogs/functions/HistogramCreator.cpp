#include "HistogramCreator.h"
#include <QProcess>
#include <QProgressBar>
#include <QProgressDialog>
#include <QDateTime>
#include <QVBoxLayout>
#include <QImageReader>
#include <QVector>
#include <QComboBox>
#include <string>
#include "../dialogs/general/ImageViewerDialog.h"
#include "../helpers/CSVReader.h"

using namespace std;

HistogramCreator::HistogramCreator(QString mPythonBinPath, QString mOMSensPath, QString mOMSensResultsPath, QWidget *pParent) : QDialog(pParent)
{
    executablePath = mPythonBinPath;
    librariesPath  = mOMSensPath;
    resultsPath    = mOMSensResultsPath;

    // Dialog settings
    setMinimumWidth(410);
    setWindowTitle("Histogram Creator");

    CSVReader *csv_reader = new CSVReader();
    // TODO: for reuse in different contexts (apart from multiparameter sweep), these 3 files must be generated!
    QVector<QString> parameters = csv_reader->getColumnsNames(mOMSensResultsPath + "/" + "results/" + "parameters_run.csv");
    QVector<QString> variables = csv_reader->getColumnsNames(mOMSensResultsPath + "/" + "results/" + "variables.csv");
    QVector<double> time_values = csv_reader->getColumnValues(mOMSensResultsPath + "/" + "results/runs/std_run.csv", "time");

    // Layout
    QVBoxLayout *pMainLayout = new QVBoxLayout;

    // Time value
    int precisionVal = 3;
    double min = time_values[0];
    double max = time_values[time_values.size() - 1];
    QString min_str = QString::fromStdString(std::to_string(min).substr(0, std::to_string(min).find(".") + precisionVal + 1));
    QString max_str = QString::fromStdString(std::to_string(max).substr(0, std::to_string(max).find(".") + precisionVal + 1));
    QString str = "Time: (Min=" + min_str + ", Max=" + max_str + ")";
    QLabel *time_label = new QLabel(str);
    options_time_box = new QComboBox;
    options_time_box->setEditable(true);
    QHBoxLayout *row1 = new QHBoxLayout;
    row1->addWidget(time_label);
    row1->addWidget(options_time_box);
    pMainLayout->addItem(row1);

    // Show histogram for variable
    QLabel *options_variables_label = new QLabel("Variable: ");
    mpButtonBox = new QDialogButtonBox;
    mpButtonBox->addButton("Show", QDialogButtonBox::AcceptRole);
    connect(mpButtonBox, &QDialogButtonBox::accepted, this, &HistogramCreator::showHistogramVariable);
    options_variables_box = new QComboBox;
    for (QString varName: variables) {
        options_variables_box->addItem(varName);
    }
    QHBoxLayout *row2 = new QHBoxLayout;
    row2->addWidget(options_variables_label);
    row2->addWidget(options_variables_box);
    row2->addWidget(mpButtonBox);
    pMainLayout->addItem(row2);

    // Show historam for parameter
    QLabel *options_parameters_label = new QLabel("Parameter: ");
    options_parameters_box = new QComboBox;
    for (QString parameterName : parameters) {
        options_parameters_box->addItem(parameterName);
    }
    QDialogButtonBox *mpButtonBoxParameters = new QDialogButtonBox;
    mpButtonBoxParameters->addButton("Show", QDialogButtonBox::AcceptRole);
    connect(mpButtonBoxParameters, &QDialogButtonBox::accepted, this, &HistogramCreator::showHistogramParameter);
    QHBoxLayout *row3 = new QHBoxLayout;
    row3->addWidget(options_parameters_label);
    row3->addWidget(options_parameters_box);
    row3->addWidget(mpButtonBoxParameters);
    pMainLayout->addItem(row3);

    // Layout settings
    setLayout(pMainLayout);
}

void HistogramCreator::showHistogramParameter()
{
    showHistogramVariable();
}

void HistogramCreator::showHistogramVariable()
{
    QString fileNamePath = resultsPath + "/results/" + "plots/"
            + "h_"
            + QString::number(options_time_box->currentIndex())
            + "_" + QString::number(options_parameters_box->currentIndex())
            + ".png";

    // Check if PNG is available. If it is not, generate it
    QImageReader reader(fileNamePath);
    const QImage newImage = reader.read();
    if (newImage.isNull()) {
        makePNG(fileNamePath);
    }
    ImageViewerDialog *pImageViewer = new ImageViewerDialog(fileNamePath, this);
    pImageViewer->show();
}

int HistogramCreator::makePNG(QString png_filename_path)
{
    // Get parameters
    QString scriptPathBaseDir = librariesPath;
    QString scriptPath        = librariesPath + "callable_methods/plot_histogram.py";
    QString pythonBinPath     = executablePath;

    QString args = "--filename_path=" + png_filename_path
            + " " + "--parameter=" + options_parameters_box->currentText()
            + " " + "--time_value=" + options_time_box->currentText()
            + " " + "--runs_path=" + resultsPath + "/results/runs";

    // GENERATE COMMAND FROM SELECTED PARAMETERS


    // RUN PROCESS
    QString command = pythonBinPath + " " + scriptPath + " " + args;
    QProcess pythonScriptProcess;
    pythonScriptProcess.setWorkingDirectory(scriptPathBaseDir);

    // Initialize dialog showing progress
    QString progressDialogText = progressDialogTextForCurrentTime();
    QProgressDialog *dialog = new QProgressDialog(progressDialogText, "Cancel", 0, 0, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);

    connect(&pythonScriptProcess, SIGNAL(finished(int)), dialog, SLOT(close()));
    connect(dialog, SIGNAL(canceled()), &pythonScriptProcess, SLOT(kill()));

    // Start process
    pythonScriptProcess.start(command);
    // Show dialog with progress
    dialog->exec();
    // Wait for the process to finish in the case that we cancel the process and it doesn't have time to finish correctly
    pythonScriptProcess.waitForFinished(3000);
    // See if the process ended correctly
    QProcess::ExitStatus exitStatus = pythonScriptProcess.exitStatus();
    int exitCode = pythonScriptProcess.exitCode();
    bool processEndedCorrectly = (exitStatus == QProcess::NormalExit) && (exitCode == 0);
    return processEndedCorrectly;
}

QString HistogramCreator::progressDialogTextForCurrentTime()
{
    QDateTime currentTime = QDateTime::currentDateTime();
    QString date = currentTime.toString("dd/MM/yyyy");
    QString h_m_s = currentTime.toString("H:m:s");
    QString scriptRunStartString = "(started on " + date + " at " + h_m_s + ")";
    QString progressDialogText = "Running python script... " + scriptRunStartString;
    return progressDialogText;
}