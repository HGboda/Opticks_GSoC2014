/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include "AppVerify.h"
#include "Gui.h"
#include <QtGui/QCheckBox>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QSpinBox>
#include <QtGui/qapplication.h>
#include <QtGui/QPushButton>
#include <Qt/qevent.h>
#include <QtGui/QMessageBox>
#include "Interpolation.h"



 Gui::Gui( QWidget* pParent, const char* pName, bool modal)
: QDialog(pParent)
{
	setModal( FALSE );
	resize(400, 400);
	setWindowTitle(QString::fromStdString("prova"));
	
	mpRunButton = new QPushButton( "Run the Plug-In", this );
	mpDEMspacing = new QDoubleSpinBox(this);

	mpLASListCombo = new QComboBox(this);
	mpLASListCombo->setFixedSize(200,20);

	// Layout
    QGridLayout* pGrid = new QGridLayout(this);
    pGrid->setMargin(0);
    pGrid->setSpacing(5);
	pGrid->addWidget(mpLASListCombo, 0, 0,1,3);
	pGrid->addWidget(mpDEMspacing, 1, 1);
	pGrid->addWidget(mpRunButton, 2, 2);
	
	// Connections
	VERIFYNRV(connect(mpRunButton, SIGNAL( clicked() ), this, SLOT( RunApplication() )));

	init();
}


/*
*  Destroys the object and frees any allocated resources
*/
Gui::~Gui()
{
	/* Service<DesktopServices> pDesktop;
	 SpatialDataWindow* pScaledWindow = dynamic_cast<SpatialDataWindow*>( pDesktop->getWindow(
      "prova", SPATIAL_DATA_WINDOW ) );
   if ( pScaledWindow != NULL )
   {
      pDesktop->deleteWindow( pScaledWindow );
      pScaledWindow = NULL;
   }*/
}


void Gui::init()
{

   mPointCloudNames = pModel->getElementNames("PointCloudElement");
   
   int ii=0;
   for (unsigned int i = 0; i < mPointCloudNames.size(); i++)
   {
	  size_t pos;
	  std::string file_ext;      	  
	  pos = mPointCloudNames[i].find_last_of(".");
	  file_ext = mPointCloudNames[i].substr(pos);	  

	  if (file_ext.compare(".las") ==0) 
	  {
		  mpLASListCombo->insertItem(ii, QString::fromStdString(mPointCloudNames[i]));	
		  ii++;
	  }
   }
   button_cont = 0;

   mpDEMspacing ->setMinimum(1);
   mpDEMspacing ->setMaximum(10);
}


void Gui::RunApplication()
{
	button_cont++;
	mpRunButton->setEnabled(false);
	std::string las_name = mPointCloudNames.at(mpLASListCombo->currentIndex());
    pElement = dynamic_cast<PointCloudElement*> (pModel->getElement(las_name, "", NULL ));
	Interpolation interp = Interpolation();
	float post_spacing = mpDEMspacing->value();//5.0f;//5.0f
	Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> dem;
    dem = interp.generate_DEM( pElement, post_spacing) ;
	std::string path = "C:/Users/Roberta/Desktop/Results/";
	interp.print_DEM_on_file( std::string(path) + "dem_from_gui.txt", dem);
	draw_raster_from_eigen_mat ("dem "+StringUtilities::toDisplayString(button_cont), dem, pElement);
	mpRunButton->setEnabled(true);
}


bool Gui::draw_raster_from_eigen_mat (std::string name, Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> eigen_matrix, PointCloudElement* pElement)
{
	const float badVal = -9999.f;
	
	RasterElement* pDemOut = RasterUtilities::createRasterElement(name,  static_cast<int>(eigen_matrix.rows()), static_cast<int>(eigen_matrix.cols()), FLT4BYTES, true, pElement);
	   if (pDemOut == NULL)
	   {
		   warning_msg += "Unable to create DEM raster ("+ name +").\n";
		   return false;
	   }
	   pDemOut->getStatistics()->setBadValues(std::vector<int>(1, (int)badVal));
	   FactoryResource<DataRequest> pReq;
	   pReq->setWritable(true);
	   DataAccessor racc(pDemOut->getDataAccessor(pReq.release()));
	
	   for (int row = 0; row < eigen_matrix.rows(); row++)
	   {
			for (int col = 0; col < eigen_matrix.cols(); col++)
			{
			if (!racc.isValid())
			{
				warning_msg += "Error writing output raster(" + name + ").";
				return false;
			}
			*reinterpret_cast<float*>(racc->getColumn()) = eigen_matrix(row, col);
			racc->nextColumn();
			}
			racc->nextRow();
	   }
	   pDemOut->updateData();
	 
	   SpatialDataView* pView = ((SpatialDataWindow*)Service<DesktopServices>()->createWindow(name, SPATIAL_DATA_WINDOW))->getSpatialDataView();
       pView->setPrimaryRasterElement(pDemOut);
       {
         UndoLock lock(pView);
         pView->createLayer(RASTER, pDemOut);
	   }
	return true;
}