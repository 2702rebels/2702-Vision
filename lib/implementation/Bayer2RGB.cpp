#include <cassert>

#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>

// MatRGB must be initialized to same size as matBayer and must be of type
// CV_8UC3
/*! \brief Converts a bayer material to an RGB scheme.
 * \param matRGB - Must be initialized to same size as matBayer and must be of
 * type CV_8UC3, will be written to as the result of the conversion. \param
 * matBayer - The bayer material to convert.
 */
void Bayer2RGB(cv::Mat &matRGB, cv::Mat const &matBayer) {
  int nRows = matBayer.rows;
  int nLoopRows =
      nRows - 1; // rows that have normal pixels .. i.e not special cases
  int nCols = matBayer.cols;
  int nLoopCols =
      nCols - 1; // columns that have normal pixels .. i.e. not special cases

  assert(matRGB.rows == matBayer.rows && matRGB.cols == matBayer.cols);
  assert(matRGB.type() == CV_8UC3 && matBayer.type() == CV_8UC1);
  assert((matBayer.rows & 1) == 0 && (matBayer.cols & 1) == 0);

  uint8_t *pu8BGR = matRGB.ptr(0);
  uint8_t const *pu8Cur = matBayer.ptr(0);

  // Handle 1st row GRGR...
  *pu8BGR++ = pu8Cur[nCols];
  *pu8BGR++ = pu8Cur[0];
  *pu8BGR++ = pu8Cur[1];
  pu8Cur++;

  for (int nCol = 1; nCol < nLoopCols; nCol++, pu8Cur++) {
    if (nCol & 1) { // red
      *pu8BGR++ = (pu8Cur[nCols - 1] + pu8Cur[nCols + 1]) / 2;
      *pu8BGR++ = (pu8Cur[-1] + pu8Cur[1] + pu8Cur[nCols]) / 3;
      *pu8BGR++ = pu8Cur[0];
    } else {
      *pu8BGR++ = pu8Cur[nCols];
      *pu8BGR++ = pu8Cur[0];
      *pu8BGR++ = (pu8Cur[-1] + pu8Cur[1]) / 2;
    }
  }

  *pu8BGR++ = pu8Cur[nCols - 1];
  *pu8BGR++ = (pu8Cur[-1] + pu8Cur[nCols]) / 2;
  *pu8BGR++ = pu8Cur[0];
  pu8Cur++;

  // #pragma omp parallel for
  for (int nRow = 1; nRow < nLoopRows; nRow++) {
    pu8Cur = matBayer.ptr(nRow);
    pu8BGR = matRGB.ptr(nRow);

    if (nRow & 1) { // BGBGBG
      *pu8BGR++ = pu8Cur[0];
      *pu8BGR++ = (pu8Cur[-nCols] + pu8Cur[1] + pu8Cur[nCols]) / 3;
      *pu8BGR++ = (pu8Cur[-nCols + 1] + pu8Cur[nCols + 1]) / 2;
    } else { // GRGRGR
      *pu8BGR++ = (pu8Cur[-nCols] + pu8Cur[nCols]) / 2;
      *pu8BGR++ = pu8Cur[0];
      *pu8BGR++ = pu8Cur[1];
    }
    pu8Cur++;
    for (int nCol = 1; nCol < nLoopCols; nCol++, pu8Cur++) {
      // The Bayer Pattern is a repeating 2x2 grid  of
      // GR
      // BG
      // and there are 4 distinct algorithms for computing the missing colors
      // Determine pixel encoding type from least significant bits
      // the encoding will be G(00) R(01) B(10) G(11)

      uint8_t u8CT = ((nRow & 1) << 1) | (nCol & 1);
      switch (u8CT) {
      case 0: // Green on Red Row
        *pu8BGR++ = (pu8Cur[-nCols] + pu8Cur[nCols]) / 2;
        *pu8BGR++ = pu8Cur[0];
        *pu8BGR++ = (pu8Cur[-1] + pu8Cur[1]) / 2;
        break;
      case 1: // Red
        *pu8BGR++ = (pu8Cur[-nCols - 1] + pu8Cur[-nCols + 1] +
                     pu8Cur[nCols - 1] + pu8Cur[nCols + 1]) /
                    4;
        *pu8BGR++ =
            (pu8Cur[-nCols] + pu8Cur[-1] + pu8Cur[1] + pu8Cur[nCols]) / 4;
        *pu8BGR++ = pu8Cur[0];
        break;
      case 2: // Blue
        *pu8BGR++ = pu8Cur[0];
        *pu8BGR++ =
            (pu8Cur[-nCols] + pu8Cur[-1] + pu8Cur[1] + pu8Cur[nCols]) / 4;
        *pu8BGR++ = (pu8Cur[-nCols - 1] + pu8Cur[-nCols + 1] +
                     pu8Cur[nCols - 1] + pu8Cur[nCols + 1]) /
                    4;
        break;
      case 3: // Green on Blue Row
        *pu8BGR++ = (pu8Cur[-1] + pu8Cur[1]) / 2;
        *pu8BGR++ = pu8Cur[0];
        *pu8BGR++ = (pu8Cur[-nCols] + pu8Cur[nCols]) / 2;
        break;
      }
    }
    if (nRow & 1) { // BGBGBG...
      *pu8BGR++ = pu8Cur[-1];
      *pu8BGR++ = pu8Cur[0];
      *pu8BGR++ = (pu8Cur[-nCols] + pu8Cur[nCols]) / 2;
    } else { // GRGRGR...
      *pu8BGR++ = (pu8Cur[-nCols - 1] + pu8Cur[nCols - 1]) / 2;
      *pu8BGR++ = (pu8Cur[-nCols] + pu8Cur[-1] + pu8Cur[nCols]) / 3;
      *pu8BGR++ = pu8Cur[0];
    }
    pu8Cur++;
  }

  // Need to handle last row...
}

/*! \brief Converts a bayer material to half RGB scheme.
 * \param matRGB - must be initialized to same size as matBayer and be of type
 * CV_8UC3, will be written to as the result of the conversion. \param matBayer
 * - The bayer material to convert.
 */
void Bayer2HalfRGB1(cv::Mat &matRGB, cv::Mat const &matBayer) {
  assert(matRGB.rows == matBayer.rows / 2 && matRGB.cols == matBayer.cols / 2);
  assert(matRGB.type() == CV_8UC3 && matBayer.type() == CV_8UC1);
  assert((matBayer.rows & 1) == 0 && (matBayer.cols & 1) == 0);

  int nRows = matBayer.rows / 2;
  int nLoopRows =
      nRows - 1; // rows that have normal pixels .. i.e not special cases
  int nCols = matBayer.cols / 2;
  int nLoopCols =
      nCols - 1; // columns that have normal pixels .. i.e. not special cases

  int nInCols = matBayer.cols;

  uint8_t const *p = matBayer.ptr(0);
  uchar *ppix = matRGB.ptr(0, 0);
  uint8_t ug;
  uint8_t const *p2 = p + nInCols;
  for (int r = 0; r < nRows; r++, p += (nInCols * 1), p2 += (nInCols * 1)) {
    for (int c = 0; c < nCols; c++) {
#if 0
//					ug = ((uint16_t)p[c + 1] + p[c + m_nCols]) / 2;
			*ppix++ = *p++;
			ug = ((uint16_t)*p++ + *p2++)/2;
			*ppix++ = ug;
			*ppix++ = +p2++;
#else
      //				ug = ((uint16_t)p[c] + p[c + m_nCols +
      //1]) / 2; 				uint8_t ug = p[c];
      *ppix++ = *p2++;
      ug = ((uint16_t)*p++ + *p2++) / 2;
      *ppix++ = ug;
      *ppix++ = *p++;
//			p += 2;
//			p2 += 2;
#endif
    }
  }
}

/*! \brief Converts a bayer material to half RGB scheme.
 * \param matRGB - must be initialized to same size as matBayer and be of type
 * CV_8UC3, will be written to as the result of the conversion. \param matBayer
 * - The bayer material to convert.
 */
void Bayer2HalfRGB(cv::Mat &matRGB, cv::Mat const &matBayer) {
  assert((matRGB.cols & 0x7) == 0);
  assert(matRGB.rows == matBayer.rows / 2 && matRGB.cols == matBayer.cols / 2);
  assert(matRGB.type() == CV_8UC3 && matBayer.type() == CV_8UC1);
  assert((matBayer.rows & 1) == 0 && (matBayer.cols & 1) == 0);

  int nRows = matBayer.rows / 2;
  int nLoopRows =
      nRows - 1; // rows that have normal pixels .. i.e not special cases
  int nCols = matBayer.cols / 2;
  int nLoopCols =
      nCols - 1; // columns that have normal pixels .. i.e. not special cases

  int nInCols = matBayer.cols;

  // p  = BGBGBG...
  // p2 = GRGRGR...
  uint32_t const *p = (uint32_t const *)matBayer.ptr(0);
  uint32_t *ppix = (uint32_t *)matRGB.ptr(0, 0);
  uint8_t ug;
  uint32_t const *p2 = p + nInCols / 4;
  for (int r = 0; r < nRows; r++, p += (nInCols / 4), p2 += (nInCols / 4)) {
    for (int c = 0; c < nCols; c += 4) {
      uint32_t u32Bayer1 = *p++;
      uint32_t u32Bayer2 = *p2++;

      uint32_t u32RGB = (u32Bayer2 & 0x0000ffff) |
                        ((u32Bayer1 & 0x0000ff00) << 8) |
                        ((u32Bayer2 & 0x00ff0000) << 8);
      *ppix++ = u32RGB;

      u32Bayer2 = *p2++;

      u32RGB = (u32Bayer1 >> 16) | (u32Bayer2 << 16);

      *ppix++ = u32RGB;

      u32Bayer1 = *p++;

      u32RGB = ((u32Bayer1 >> 8) & 0xFF) | ((u32Bayer2 >> 8) & 0x00FFFF00) |
               (u32Bayer1 & 0xFF000000);

      *ppix++ = u32RGB;
    }
  }
}
